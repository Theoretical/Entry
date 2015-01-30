#include <enet/enet.h>
#include <intlib/base64.h>
#include <intlib/blowfish.h>
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>
#include <map>
#include <stdio.h>
#include <time.h>
#include <vector>
#include "common.h"
#include "Packets.h"

#define PACKET_CALLBACK(name) void name(ENetPeer* peer, int channel, unsigned char* packet, int length)
typedef void(__cdecl* OnPacketProcessor_t)(ENetPeer* peer, int channel, unsigned char* packet, int size);

BlowFish* g_Blowfish;
uint32 g_NetId = 0;
uint32 g_SummonerId = 0;
uint32 g_CappedPoints;
uint64 g_UserId = 0;
uint32 g_GameTimer = 0;
uint32 g_lastSkillLeveled;
bool g_Connected = false;
bool g_Positive = true;
bool g_IsOrder;
bool g_IsOdin;
bool g_GameStarted = false;
char g_PlayerName[64];
std::vector<uint32> g_CapPoints;
std::map<uint8, OnPacketProcessor_t> g_PacketProcessors;
std::map<uint8, char*> g_packetDefs;

void BuildPacketTables();
void SendEnet(ENetPeer* peer, unsigned char* buffer, uint32 size, int channel = CHL_C2S);
void on_connect(ENetPeer* peer);
void on_receive(ENetPeer* peer, ENetPacket* packet, uint8 channelId);
void send_surrender(ENetPeer* peer);
void send_move(ENetPeer* peer, bool mode);
void send_buy_item(ENetPeer* peer, uint32 id);
void send_level_skill(ENetPeer* peer);

uint32 GetTicks()
{
	return GetTickCount();
}


int main(int argc, char** argv)
{
	if (argc < 4)
		return 0;

	SetConsoleTitleA("Entry - Taking the referral service to a new level.");
	ENetAddress address;
	ENetEvent event;
	
	BuildPacketTables();

	// Startup ENet
	if (enet_initialize() != 0)
	{
		printf("Failed to start up ENet.\n");
		return 0;
	}

	atexit(enet_deinitialize);

	// Initialize ENetAddress for client
	ENetHost* client = enet_host_create(NULL, 6, 0, 0);

	if (!client)
	{
		printf("Failed to create enet host.\n");
		return 0;
	}

	printf("Setting host to: %s:%i\n", argv[1], atoi(argv[2]));
	enet_address_set_host(&address, argv[1]);
	address.port = atoi(argv[2]);

	std::string strKey = base64_decode(argv[3]);
	g_Blowfish = new BlowFish((unsigned char*)strKey.c_str(), 16);
	g_SummonerId = atoi(argv[4]);

	// Connect to server.
	ENetPeer* peer = enet_host_connect(client, &address, 64);
	client->headerFlags = 255;

	if (!peer)
	{
		printf("No available peers for server.\n");
		return 0;
	}

	bool printed = false;
	uint32 last_packet = GetTicks();
	uint32 move_timer = GetTicks();

	while (true) 
	{
		if (GetTicks() - last_packet >= 10000 && !g_Connected)
			ExitProcess(0);

		int result = enet_host_service(client, &event, 10);

		if (result > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				client->headerFlags = ENET_PROTOCOL_HEADER_FLAG_SENT_TIME;
				on_connect(peer);
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				on_receive(peer, event.packet, event.channelID);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				printf("Disconnected from server: Ticks (%u)!\n", (GetTickCount() - g_GameTimer) / 1000);
			}
		}

		if (g_IsOdin) 
		{
			if (g_NetId != 0 && g_CapPoints.size() > 0)
			{
				if (g_IsOrder)
				{
					// Yay we have points to cap.
					if (g_CappedPoints == 0 && GetTicks() - last_packet >= 3000)
					{
						last_packet = GetTicks();
						if (!printed)
						{
							printf("Capping point A\n");
							printed = true;
						}
						CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(0));
						SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
						delete cp;
					}
					else if (g_CappedPoints == 8 && GetTicks() - last_packet >= 3000)
					{
						last_packet = GetTicks();
						if (GetTickCount() - g_GameTimer >= 170000)
						{
							if (!printed)
							{
								printf("Capping point E\n");
								printed = true;
							}
							CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(4));
							SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
							delete cp;
						}
						else
						{
							printed = false;
							send_move(peer, g_Positive);
							send_level_skill(peer);
							g_Positive = !g_Positive;						
						}
					}
				}
				else
				{
					if (GetTickCount() - g_GameTimer >= 6000)
					{
						g_GameTimer = GetTickCount();
						send_move(peer, g_Positive);
						send_level_skill(peer);
						g_Positive = !g_Positive;
					}
				}
			}
		}
		else
		{
			if (GetTicks() - g_GameTimer >= 1200000 && g_GameTimer != 0)
			{
				send_surrender(peer);
			}
			else if (g_GameTimer != 0)
			{
				printf("Current tick: %u | Elapsed seconds: %u | 20 minutes = 1200000 ticks\r", GetTicks() - g_GameTimer, (GetTicks() - g_GameTimer) / 1000);
				if (GetTickCount() - move_timer >= 6000)
				{
					move_timer = GetTickCount();
					send_move(peer, g_Positive);
					send_level_skill(peer);
					g_Positive = !g_Positive;
				}
			}
		}
	}
	return 0;
}

void on_connect(ENetPeer* peer)
{
	g_Connected = true;
	printf("Connected to server.\n");
	KeyCheck* key = new KeyCheck();
	
	unsigned char* blowfishKey = g_Blowfish->getKey();
	key->partialKey[0] = blowfishKey[1];
	key->partialKey[1] = blowfishKey[2];
	key->partialKey[2] = blowfishKey[3];
	printf("%X %X %X\n", blowfishKey[1], blowfishKey[2], blowfishKey[3]);
	key->trash = 41300265;
	key->trash2 = 51685968;
	key->userId = g_SummonerId;
	key->checkId = g_Blowfish->Encrypt(key->userId);

	ENetPacket* packet = enet_packet_create(reinterpret_cast<unsigned char*>(key), sizeof(KeyCheck), ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, CHL_HANDSHAKE, packet);

	delete key;
}

void send_query(ENetPeer* peer)
{
	unsigned char queryReq[] = { 0x14, 0, 0, 0, 0 };
	SendEnet(peer, queryReq, 5);
}

PACKET_CALLBACK(ProcessKeyCheck)
{
	if (channel != CHL_HANDSHAKE || g_UserId != 0)
		return;

	KeyCheck* keyCheck = reinterpret_cast<KeyCheck*>(packet);
	g_UserId = keyCheck->checkId;
	g_Blowfish->Decrypt(g_UserId);
	printf("Got userId for client: (%i). Player No: %i\n", g_UserId, keyCheck->playerNo);
}

PACKET_CALLBACK(ProcessWorldSendGameNumber)
{
	WorldSendGameNumber* game = reinterpret_cast<WorldSendGameNumber*>(packet);
	printf("[User: %s][Game Id: %i]\n", game->server, game->gameId);
	strcpy(g_PlayerName, (char*)game->server);
	send_query(peer);
}

PACKET_CALLBACK(ProcessQueryStatusAns)
{
	QueryStatus* status = reinterpret_cast<QueryStatus*>(packet);
	if (g_GameStarted)
		return;

	if (status->ok)
	{
		printf("Sending Version\n");
		// We should be able to do shit now.
		SynchVersion* version = new SynchVersion();
		SendEnet(peer, (uint8*)version, sizeof(SynchVersion));

		delete version;
	}
	else
	{
		send_query(peer);
	}
}

PACKET_CALLBACK(ProcessSyncVersionAns)
{
	if (!g_GameStarted)
	{
		g_GameStarted = true;
		printf("Got Version SyncAns\n");
		SynchVersionAns* answer = reinterpret_cast<SynchVersionAns*>(packet);

		printf("Setting up for game mode: %s\n", (char*)answer->gameMode);
		
		for (int i = 0; i < 10; ++i)
		{
			if (answer->players[i].userId == g_SummonerId)
			{
				g_IsOrder = answer->players[i].teamId == 0x64;
			}
		}

		if (!stricmp((char*)answer->gameMode, "ODIN"))
			g_IsOdin = true;

		uint8 clientReady[] = { 0x64, 0x00, 0x00, 0x00, 0x00 };
		SendEnet(peer, clientReady, 5, CHL_LOADING_SCREEN);

		PingLoadInfo* ping = new PingLoadInfo();

		uint8* pingPtr = (uint8*)ping;
		uint32 pingLength = sizeof(PingLoadInfo);

		ping->header.cmd = PKT_C2S_Ping_Load_Info;
		ping->loaded = 100;
		ping->ping = 2;
		ping->userId = g_SummonerId;

		SendEnet(peer, (uint8*)ping, sizeof(PingLoadInfo));

		delete ping;

		uint8 charLoaded[] = { 0xBE, 0x00, 0x00, 0x00, 0x00 };
		SendEnet(peer, charLoaded, 5);
		uint8 startGame[] = { 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		SendEnet(peer, startGame, sizeof(startGame));
	}
}

PACKET_CALLBACK(ProcessHeroSpawn)
{
	HeroSpawn* info = reinterpret_cast<HeroSpawn*>(packet);
	if (!stricmp(info->name, g_PlayerName) && g_NetId == 0)
	{
		g_NetId = info->netId;
		//g_IsOrder = info->teamIsOrder;
		printf("Spawning player: %s Hero: %s. Team: %s. NetId: %X\n", info->name, info->type, !info->teamIsOrder ? "ORDER" : "CHAOS", info->netId);
		g_GameTimer = GetTicks();

		send_buy_item(peer, 0x3E9); // boots.
		for (int i = 0; i < 4; ++i)
			send_buy_item(peer, 0x7D3); // pots.
		
		send_buy_item(peer, 0xD0C); // trinket
		send_level_skill(peer);
	}
}

PACKET_CALLBACK(ProcessMinionSpawn)
{
	MinionSpawn* ms = reinterpret_cast<MinionSpawn*>(packet);
	char* minionName = (char*)(packet + 54);

	if (!stricmp(minionName, "OdinNeutralGuardian"))
	{
		printf("Found point(%s) with netId(%d) from first batch packet\n", minionName, ms->netId);
		g_CapPoints.push_back(ms->netId);
	}
}

PACKET_CALLBACK(ProcessBatch)
{
	u_char* ptr = reinterpret_cast<u_char*>(packet);
	int totalPackets = ptr[1];
	int packetSize = ptr[2];
	ptr += 3;

	unsigned char prevCommand = ptr[0];
	PacketHeader* ptrHeader = (PacketHeader*)ptr;


	if (g_PacketProcessors.find(ptrHeader->cmd) != g_PacketProcessors.end())
		g_PacketProcessors[ptrHeader->cmd](peer, channel, ptr, packetSize);

	ptr += packetSize;

	for (int i = 2; i < totalPackets + 1; ++i)
	{
		unsigned char flagsAndLength = ptr[0]; 
		unsigned char size = flagsAndLength >> 2;
		unsigned char additionalByte = ptr[1];
		unsigned char command;
		unsigned char buffer[8192];

		++ptr;

		if (flagsAndLength & 1) 
		{ 
			++ptr;
			command = prevCommand;
		}
		else 
		{
			command = ptr[0];
			++ptr;
			if (flagsAndLength & 2)
				ptr += 1;
			else 
				ptr += 4;
		}

		if (size == 0x3F) 
		{
			size = ptr[0];
			++ptr;
		}

		buffer[0] = command;
		memcpy(buffer + 1, ptr, size);

		ptrHeader = (PacketHeader*)buffer;
		//printf("Next packet in batch: %02X (%s)\n", ptrHeader->cmd, GetPacketName(ptrHeader->cmd));

		if (g_PacketProcessors.find(ptrHeader->cmd) != g_PacketProcessors.end())
			g_PacketProcessors[ptrHeader->cmd](peer, channel, buffer-4, packetSize);

		ptr += size;

		prevCommand = command;
	}
}

PACKET_CALLBACK(ProcessChatboxMessage)
{
	return;
	ChatMessage* cm = reinterpret_cast<ChatMessage*>(packet);

	printf("Message: %s\n", cm->getMessage());
	if (strstr(cm->getMessage(), "move")) {
		Move* m = new Move(g_NetId);
		m->type = 7;

		SendEnet(peer, (unsigned char*)m, sizeof(Move));
		delete m;
	}
	else if (strstr(cm->getMessage(), "cap"))
	{
		srand(time(NULL));
		int id = rand() % g_CapPoints.size();
		CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(id));

		SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
		delete cp;
	}
}

PACKET_CALLBACK(ProcessRemoveVisionBuff)
{
	if (!g_IsOdin)
		return;

	if (g_IsOrder)
	{
		if (g_CappedPoints <= 1)
		{
			printf("Capping point B\n");
			CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(1));

			SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
			delete cp;
		}
		else if (g_CappedPoints == 3)
		{
			printf("Capping point C\n");
			CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(2));

			SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
			delete cp;
		}
		else if (g_CappedPoints == 5)
		{
			printf("Capping point D\n");
			CapPoint* cp = new CapPoint(g_NetId, g_CapPoints.at(3));

			SendEnet(peer, (unsigned char*)cp, sizeof(CapPoint));
			delete cp;
		}
		else if (g_CappedPoints == 7)
		{
			g_GameTimer = GetTickCount();
			printf("Waiting to cap point E...need to finish by 7mins\n");
		}
		
		g_CappedPoints++;
	}
}

PACKET_CALLBACK(ProcessGainVision)
{
	if (!g_IsOrder)
	{
		printf("Attempting to move so we're not idle!\n");
		send_move(peer, g_Positive);
		send_level_skill(peer);
		g_Positive = !g_Positive;
	}
}
void on_receive(ENetPeer* peer, ENetPacket* packet, uint8 channelId)
{
	if (packet->dataLength >= 8)
		g_Blowfish->Decrypt(packet->data, packet->dataLength - (packet->dataLength % 8));

	PacketHeader* header = reinterpret_cast<PacketHeader*>(packet->data);
//	printf("Got packet: %02X\n", header->cmd);
	if (g_PacketProcessors.find(header->cmd) != g_PacketProcessors.end())
		g_PacketProcessors[header->cmd](peer, channelId, packet->data, packet->dataLength);


}

void BuildPacketTables()
{
	g_packetDefs[PKT_KeyCheck] = "PKT_KeyCheck";
	g_packetDefs[PKT_S2C_EndSpawn] = "PKT_S2C_EndSpawn";
	g_packetDefs[PKT_S2C_SkillUp] = "PKT_S2C_SkillUp";
	g_packetDefs[PKT_S2C_AutoAttack] = "PKT_S2C_AutoAttack";
	g_packetDefs[PKT_S2C_PlayerInfo] = "PKT_S2C_PlayerInfo";
	g_packetDefs[PKT_S2C_ViewAns] = "PKT_S2C_ViewAns";
	g_packetDefs[PKT_S2C_SynchVersion] = "PKT_S2C_SynchVersion";
	g_packetDefs[PKT_S2C_StartGame] = "PKT_KeyCheck";
	g_packetDefs[PKT_S2C_StartSpawn] = "PKT_S2C_StartSpawn";
	g_packetDefs[PKT_S2C_LoadHero] = "PKT_S2C_LoadHero";
	g_packetDefs[PKT_S2C_LoadName] = "PKT_S2C_LoadName";
	g_packetDefs[PKT_S2C_LoadScreenInfo] = "PKT_S2C_LoadScreenInfo";
	g_packetDefs[PKT_ChatBoxMessage] = "PKT_ChatBoxMessage";
	g_packetDefs[PKT_S2C_QueryStatusAns] = "PKT_S2C_QueryStatusAns";
	g_packetDefs[PKT_World_SendGameNumber] = "PKT_World_SendGameNumber";
	g_packetDefs[PKT_S2C_Ping_Load_Info] = "PKT_S2C_Ping_Load_Info";
	g_packetDefs[PKT_S2C_CharStats] = "PKT_S2C_CharStats";
	g_packetDefs[PKT_S2C_HeroSpawn] = "PKT_S2C_HeroSpawn";
	g_packetDefs[PKT_S2C_MinionSpawn] = "PKT_S2C_MinionSpawn";
	g_packetDefs[PKT_S2C_PlayerInfo] = "PKT_S2C_PlayerInfo";
	g_packetDefs[PKT_S2C_TurretSpawn] = "PKT_S2C_TurretSpawn";
	g_packetDefs[PKT_S2C_LevelPropSpawn] = "PKT_S2C_LevelPropSpawn";
	g_packetDefs[PKT_S2C_BuyItemAns] = "PKT_S2C_BuyItemAns";
	g_packetDefs[PKT_S2C_GameTimer] = "PKT_S2C_GameTimer";
	g_packetDefs[PKT_S2C_GameTimerUpdate] = "PKT_S2C_GameTimerUpdate";
	g_packetDefs[PKT_Batch] = "PKT_Batch";

	g_PacketProcessors[PKT_KeyCheck] = ProcessKeyCheck;
	g_PacketProcessors[PKT_World_SendGameNumber] = ProcessWorldSendGameNumber;
	g_PacketProcessors[PKT_S2C_QueryStatusAns] = ProcessQueryStatusAns;
	g_PacketProcessors[PKT_S2C_SynchVersion] = ProcessSyncVersionAns;
	g_PacketProcessors[PKT_S2C_HeroSpawn] = ProcessHeroSpawn;
	g_PacketProcessors[PKT_S2C_MinionSpawn] = ProcessMinionSpawn;
	g_PacketProcessors[PKT_Batch] = ProcessBatch;
	g_PacketProcessors[PKT_ChatBoxMessage] = ProcessChatboxMessage;
	g_PacketProcessors[0x33] = ProcessRemoveVisionBuff;
	g_PacketProcessors[0xAD] = ProcessGainVision;
}

char* GetPacketName(uint8 id)
{
	if (g_packetDefs.find(id) != g_packetDefs.end())
		return g_packetDefs[id];

	return "Unknown packet!";
}

void SendEnet(ENetPeer* peer, unsigned char* buffer, uint32 size, int channel)
{
	g_Blowfish->Encrypt(buffer, size - (size % 8));

	ENetPacket* packet = enet_packet_create(buffer, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, CHL_C2S, packet);
}

void send_surrender(ENetPeer* peer)
{
	printf("Attempting to surrender.\n");

	Surrender* surrender = new Surrender(1);
	SendEnet(peer, (unsigned char*)surrender, sizeof(Surrender));
	delete surrender;
}

void send_move(ENetPeer* peer, bool mode)
{
	struct move
	{
		PacketHeader header;
		uint8 moveType;
		unsigned char x[4];
		unsigned char z[4];
		uint32 unk0;
		uint8 waypointCount;
		uint32 netId;
		unsigned char waypointData[20];
	};

	if (!g_IsOdin)
	{
		move* m = new move();
		m->header.cmd = PKT_C2S_MoveReq;
		m->header.netId = g_NetId;
		m->moveType = 2; // MOVE_ATTACK
		memcpy(m->x, "\xAA\xEF\x15\x46", 4);
		memcpy(m->z, "\x8c\xc1\x17\x46", 4);
		m->unk0 = 0;
		m->waypointCount = 10;
		m->netId = g_NetId;
		// we have no reason to decipher this static data.
		unsigned char waypoints[] = { 0x20, 0x6B, 0xF2, 0x6E, 0xF2, 0xDB, 0xF5, 0x8F, 0xF4, 0x3A, 0xFB, 0x52, 0xFA, 0xE9, 0xFB, 0x7D, 0x16, 0x05, 0xDC, 0x04 };
		memcpy(m->waypointData, waypoints, sizeof(waypoints));

		SendEnet(peer, (unsigned char*)m, sizeof(move));
		delete m;
		return;
	}
	
	if (mode == true)
	{
		move* m = new move();
		m->header.cmd = PKT_C2S_MoveReq;
		m->header.netId = g_NetId;
		m->moveType = 2;
		memcpy(m->x, "\x38\xDE\xFE\x45", 4);
		memcpy(m->z, "\x69\x7B\x8E\x45", 4);
		m->unk0 = 0;
		m->waypointCount = 24;
		m->netId = g_NetId;

		unsigned char waypoints[] = { 0x2C, 0xAC, 0x28, 0xC4, 0xF2, 0x57, 0xF3, 0x19, 0xF4, 0x3E, 0xF5, 0x4B, 0x32, 0x2C, 0xF5, 0x19, 0xC9, 0xF8, 0xE1, 0xF7, 0xF5, 0xF9, 0xF4, 0xF8, 0x64, 0x4B, 0xE9, 0xFB, 0x83, 0x7F, 0xFC, 0x00, 0x73, 0xFE, 0x9D, 0xFA, 0xC4, 0x01, 0x32, 0x46, 0x02, 0xFE };
		memcpy(m->waypointData, waypoints, sizeof(waypoints));

		SendEnet(peer, (unsigned char*)m, sizeof(move));
//		delete[] m->waypointData;
		delete m;
	}
	else
	{
		move* m = new move();
		m->header.cmd = PKT_C2S_MoveReq;
		m->header.netId = g_NetId;
		m->moveType = 2;
		memcpy(m->x, "\xc8\xd3\x0\x42", 4);
		memcpy(m->z, "\x80\xA1\x82\x43", 4);
		m->unk0 = 0;
		m->waypointCount = 24;
		m->netId = g_NetId;


		unsigned char waypoints[] = {0xBE, 0x02, 0x30, 0x46, 0x02, 0xCD, 0xFA, 0xAB, 0xFD, 0x02, 0xCE, 0x32, 0xB5, 0x19, 0x7F, 0xFC, 0x19, 0xE9, 0xFB, 0xB5, 0x21, 0xFB, 0x20, 0xFA, 0xD5, 0xF6, 0xED, 0xF5, 0x26, 0xF6, 0xF3, 0xF4, 0x90, 0xF5, 0x5D, 0xF4, 0xD4, 0xF2, 0x9B, 0xF2, 0x95, 0xCD };
		memcpy(m->waypointData, waypoints, sizeof(waypoints));

		SendEnet(peer, (unsigned char*)m, sizeof(move));
		//delete[] m->waypointData; 
		delete m;
	}

	unsigned char a[]= {0x77, 0x00, 0x00, 0x00, 0x00, 0x7B, 0x75, 0x00, 0x00, 0x00};
	SendEnet(peer, (unsigned char*)a, sizeof(a));
}

void send_buy_item(ENetPeer* peer, uint32 id)
{
	BuyItemReq* req = new BuyItemReq();
	req->header.cmd = PKT_C2S_BuyItemReq;
	req->header.netId = g_NetId;
	req->id = id;

	SendEnet(peer, (unsigned char*)req, sizeof(BuyItemReq));
	delete req;
}

void send_level_skill(ENetPeer* peer)
{
	if (g_lastSkillLeveled >= 4)

		g_lastSkillLeveled = 0;
	SkillUpPacket* skill = new SkillUpPacket();
	skill->header.cmd = PKT_C2S_SkillUp;
	skill->header.netId = g_NetId;
	skill->skill = g_lastSkillLeveled;

	SendEnet(peer, (unsigned char*)skill, sizeof(SkillUpPacket));
	delete skill;
}