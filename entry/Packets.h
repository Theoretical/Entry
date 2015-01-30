/*
IntWars playground server for League of Legends protocol testing
Copyright (C) 2012  Intline9 <Intline9@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _PACKETS_H
#define _PACKETS_H

#include <enet/enet.h>
#include "common.h"
#include <time.h>
#include <intlib/general.h>

#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif


struct PacketHeader
{
	PacketHeader()
	{
		netId = 0;
	}

	PacketCmd cmd;
	uint32 netId;
};


struct Surrender
{
	PacketHeader header;
	uint8 ok;

	Surrender(uint8 val)
	{
		header.netId = 0;
		header.cmd = PKT_C2S_Surrender;
		ok = val;
	}
};


struct GameHeader
{
	GameHeader()
	{
		netId = ticks = 0;
	}

	GameCmd cmd;
	uint32 netId;
	uint32 ticks;
};
typedef struct _SynchBlock {
	_SynchBlock(){}
	DWORD64 userId;
	WORD wUnk;
	DWORD skill1;
	DWORD skill2;
	BYTE bBot;
	DWORD teamId;
	BYTE name[64];
	BYTE type[64];
	BYTE rank[30];
} SynchBlock;


struct ClientReady
{
	uint32 cmd;
	uint32 playerId;
	uint32 teamId;
};

typedef struct _SynchAns
{
	PacketHeader header;
	BYTE ok;
	DWORD mapId;
	SynchBlock players[12];
	BYTE version[256];
	BYTE gameMode[128];
	BYTE unk1[512];
	BYTE unk2[245];

	BYTE ekg1[256]; //ekg.riotgames.net
	BYTE msg1[256]; //"/messages"

	WORD wUnk1; //0x50?
	BYTE ekg2[256]; //ekg.riotgames.net
	BYTE msg2[256]; //"/messages"

	WORD wUnk2; //0x50?
	BYTE ekg3[256]; //ekg.riotgames.net
	BYTE msg3[256]; //"/messages"

	WORD wUnk3; //0x50?
	DWORD dwUnk1;
	DWORD dwOpt; //0x377192
	BYTE bUnk1[0x100];
	BYTE bUnk2[11];
} SynchVersionAns;

typedef struct _PingLoadInfo
{
	PacketHeader header;
	uint32 unk1;
	uint64 userId;
	float loaded;
	float ping;
	float f3;
}PingLoadInfo;

uint8 *createDynamicPacket(uint8 *str, uint32 size);

typedef struct _LoadScreenInfo
{
	_LoadScreenInfo()
	{
		//Zero this complete buffer
		memset(this, 0, sizeof(_LoadScreenInfo));

		cmd = PKT_S2C_LoadScreenInfo;
		blueMax = redMax = 6;		
	}

	uint8 cmd;
	uint32 blueMax;
	uint32 redMax;
	uint64 bluePlayerIds[6]; //Team 1, 6 players max
	uint8 blueData[144];
	uint64 redPlayersIds[6]; //Team 2, 6 players max
	uint8 redData[144];
	uint32 bluePlayerNo;
	uint32 redPlayerNo;
} LoadScreenInfo;

typedef struct _LoadScreenPlayer
{
	static _LoadScreenPlayer* create(PacketCmd cmd, int8 *str, uint32 size)
	{
		//Builds packet buffer
		uint32 totalSize = sizeof(_LoadScreenPlayer)+size+1;
		uint8* buf = new uint8[totalSize];
		memset(buf, 0, totalSize);

		//Set defaults
		_LoadScreenPlayer *packet = (_LoadScreenPlayer *)buf;
		packet->cmd = cmd;
		packet->length = size;
		packet->userId = 0;
		packet->skinId = 0;
		memcpy(packet->getDescription(), str, packet->length);
		return packet;
	}

	static void destroy(_LoadScreenPlayer *packet)
	{
		delete [](uint8*)packet;
	}

	uint8 cmd;
	uint64 userId;
	uint32 skinId;
	uint32 length;
	uint8 description;
	uint8 *getDescription()
	{
		return &description;
	}

	uint32 getPacketLength()
	{
		return sizeof(_LoadScreenPlayer)+length;
	}
} LoadScreenPlayer;


typedef struct _KeyCheck
{
	_KeyCheck()
	{
		cmd = PKT_KeyCheck;
		playerNo = 0;
		checkId = 0;
		trash = trash2 = 0;

	}

	uint8 cmd;
	uint8 partialKey[3];   //Bytes 1 to 3 from the blowfish key for that client
	uint32 playerNo;
	uint64 userId;         //uint8 testVar[8];   //User id
	uint32 trash;
	uint64 checkId;        //uint8 checkVar[8];  //Encrypted testVar
	uint32 trash2;
} KeyCheck;

typedef struct _AttentionPing
{
	_AttentionPing()
	{

	}
	_AttentionPing(_AttentionPing *ping)
	{
		cmd = ping->cmd;
		unk1 = ping->unk1;
		x = ping->x;
		y = ping->y;
		z = ping->z;
		unk2 = ping->unk2;
		type = ping->type;
	}

	uint8 cmd;
	uint32 unk1;
	float x;
	float y;
	float z;
	uint32 unk2;
	uint8 type;
} AttentionPing;

typedef struct _AttentionPingAns
{
	_AttentionPingAns(AttentionPing *ping)
	{
		attentionPing = AttentionPing(ping);
		attentionPing.cmd = PKT_S2C_AttentionPing;
	}

	AttentionPing attentionPing;
	uint32 response;
} AttentionPingAns;

struct CameraLock
{
	PacketHeader header;
	uint8 isLock;
	uint32 padding;
};

typedef struct _ViewReq
{
	uint8 cmd;
	uint32 unk1;
	float x;
	float zoom;
	float y;
	float y2;		//Unk
	uint32 width;	//Unk
	uint32 height;	//Unk
	uint32 unk2;	//Unk
	uint8 requestNo;
} ViewReq;

struct MovementVector
{
	short x;
	short y;
	MovementVector() {
		x = 0;
		y = 0;
	}
	MovementVector(uint16 x,uint16 y) {
		this->x = x;
		this->y = y;
	}
};

struct MovementReq
{
	PacketHeader header;
	MoveType type;
	float x;
	float y;
	uint32 zero;
	uint8 vectorNo;
	uint32 netId;
	uint8 moveData;
};

struct MovementAns
{
	MovementAns()
	{
		header.cmd = PKT_S2C_MoveAns;
	}

	GameHeader header;
	uint16 ok;
	uint8 vectorNo;
	uint32 netId;
	uint8 moveData; //bitMasks + Move Vectors

	MovementVector *getVector(uint32 index)
	{
		if(index >= vectorNo/2)
			return NULL;
		MovementVector* vPoints = (MovementVector*)((DWORD)&moveData + maskCount());

		return &vPoints[index];
	}

	int maskCount() {
		float fVal = vectorNo/2;
		return ceil((fVal - 1) / 4);
	}

	static uint32 size(uint8 vectorNo)
	{
		float fVectors = vectorNo;
		int maskCount = ceil((fVectors - 1) / 4);
		return sizeof(MovementAns)+(vectorNo*sizeof(MovementVector)) + maskCount;//-1 since struct already has first moveData byte
	}

	uint32 size()
	{
		return size(vectorNo/2);
	}

	static MovementAns *create(uint32 vectorNo)
	{
		int nSize = size(vectorNo/2);
		MovementAns *packet = (MovementAns*)new uint8[nSize];
		memset(packet, 0, nSize);
		packet->header.cmd = PKT_S2C_MoveAns;
		packet->header.ticks = clock();
		packet->vectorNo = vectorNo;
		return packet;
	}

	static void destroy(MovementAns *packet)
	{
		delete [](uint8*)packet;
	}

};

struct Move
{
	PacketHeader header;
	uint8 type;
	uint32 x;
	uint32 z;
	uint32 zero;
	uint32 netid;
	uint8 zero2;
	uint32 unkn1;
	uint32 unkn2;

	Move(uint32 netId) {
		netid = netId;
		header.netId = netId;
		unkn1 = 0xF294F27C;
		unkn2 = 0xF3FBF318;
		x = 0x45c30472;
		z = 0x4601f4aa;
		zero = 0;
		zero2 = 0;
		netid = 0;
		header.cmd = PKT_C2S_MoveReq;
		header.netId = 0;
		type = 7; // 7 == PLAYER_MOVEMENT_ATTACK
	}
};

struct CapPoint
{
	PacketHeader header;
	uint32 capNetId;

	CapPoint(uint32 netId, uint32 cap)
	{
		header.cmd = (PacketCmd)0x3A;
		header.netId = netId;
		capNetId = cap;
	}
};

typedef struct _ViewAns
{
	_ViewAns()
	{
		cmd = PKT_S2C_ViewAns;
		unk1 = 0;
	}

	uint8 cmd;
	uint32 unk1;
	uint8 requestNo;
} ViewAns;


typedef struct _QueryStatus
{
	_QueryStatus()
	{
		header.cmd = PKT_S2C_QueryStatusAns;
		ok = 1;
	}
	PacketHeader header;
	uint8 ok;
} QueryStatus;

typedef struct _SynchVersion
{
	PacketHeader header;
	uint32 unk1;
	uint8 version[256]; 

	_SynchVersion()
	{
		unk1 = 0;
		header.cmd = PKT_C2S_SynchVersion;
		strcpy((char*)version, "Version 4.20.0.315 [PUBLIC]");
	}
} SynchVersion;

typedef struct _WorldSendGameNumber
{
	_WorldSendGameNumber()
	{
		header.cmd = PKT_World_SendGameNumber;
		memset(data, 0, sizeof(data1));
		memset(data, 0, sizeof(data));
		gameId = 0;
	}

	PacketHeader header;
	uint64 gameId; //_0x0000
	uint8 server[5]; //0x0008
	uint8 data1[27]; //0x000D
	uint8 data[0x80];//0x0028
} WorldSendGameNumber;


struct CharacterStats
{
	static uint32 countBits(uint32 mask, bool addMask = true)
	{
		uint32 size = 0;

		for(int i = 0; i < 32; i++)
		{
			if(mask & 1)
				size++;
			mask >>= 1;
		}
		if(addMask && size > 0)
			size++;
		return size;
	}

	uint32 getSize()
	{
		uint32 blockNo = 0;
		uint32 size = sizeof(GameHeader)+2+4;
		uint32 *offset = &mask;

		//How many blocks do we have?
		if(masterMask & MM_One)   blockNo++;
		if(masterMask & MM_Two)   blockNo++;
		if(masterMask & MM_Three) blockNo++;
		if(masterMask & MM_Four)  blockNo++;
		if(masterMask & MM_Five)  blockNo++;

		for(int i = 0, x = 0; i < blockNo; i++)
		{
			uint32 bits = countBits(offset[x], false);
			size += bits*4+4;
			x += bits+1;
		}
		return size;
	}

	static CharacterStats *create(uint32 blockNo, uint32 mask)
	{
		switch(blockNo)
		{
			case 1: return create(mask, 0, 0, 0, 0);
			case 2: return create(0, mask, 0, 0, 0);
			case 3: return create(0, 0, mask, 0, 0);
			case 4: return create(0, 0, 0, mask, 0);
			case 5: return create(0, 0, 0, 0, mask);
			default: return NULL;
		}
	}

	static CharacterStats *create(uint32 one, uint32 two, uint32 three, uint32 four, uint32 five)
	{
		//Calculate the total size needed
		uint32 size = sizeof(GameHeader)+2+4; //The header + updateNo + masterMask + netId
		size += countBits(one)*4;
		size += countBits(two)*4;
		size += countBits(three)*4;
		size += countBits(four)*4;
		size += countBits(five)*4;

		//Set the defaults
		CharacterStats *stats = (CharacterStats*)new uint8[size];
		memset(stats, 0, size);
		stats->header.cmd = (GameCmd)PKT_S2C_CharStats;
		stats->header.ticks = clock();
		stats->updateNo = 1;

		//Set the master mask
		if(one > 0)   stats->masterMask |= MM_One;
		if(two > 0)   stats->masterMask |= MM_Two;
		if(three > 0) stats->masterMask |= MM_Three;
		if(four > 0)  stats->masterMask |= MM_Four;
		if(five > 0)  stats->masterMask |= MM_Five;
		
		//Set all the masks
		uint32 *offset = &stats->mask;
		uint32 x = 0;
		offset[x] = one;
		x += countBits(one, false);
		offset[x] = two;
		x += countBits(two, false);
		offset[x] = three;
		x += countBits(three, false);
		offset[x] = four;
		x += countBits(four, false);
		offset[x] = five;

		return stats;
	}

	void destroy()
	{
		delete[]this;
	}

	void setValue(FieldMaskOne field, float value)
	{
		setValue(1, field, value);
	}

	void setValue(uint8 blockNo, uint32 field, float value)
	{
		blockNo -= 1; //We count the fields from 1 to 5
		uint32 *offset = &mask;

		//Get the offset for the block number
		uint32 x = 0;
		uint32 blocks = countBits(masterMask, false);
		if(blocks > 1)
			for(int i = 0; i < blockNo; i++)
					x += countBits(offset[x], false);

		//Get the offset for the field
		for(uint32 i = 0,  a = 0, mask = 1; i < 32; i++)
		{
			if(mask & field)
			{
				memcpy(&offset[x+a+1], &value, 4);
				a++;
			}
			mask <<= 1;
		}

	}

	GameHeader header;
	uint8 updateNo;
	uint8 masterMask;
	uint32 netId;
	uint32 mask;
};

struct MinionSpawn {
	PacketHeader header;
	//53
	uint32 unk;
	uint8 unk2;
	uint32 netId, netId2;
	uint32 position;
	uint8 unk4;
	uint8 unk5_1;
	uint8 type;
	uint8 unk5_3;
	uint8 unk5_4;
	uint8 unk6[36];
	uint32 netId3;
	uint8 unk7;
	uint64 unk8;
};

struct ChatMessage
{
	uint8 cmd;
	uint32 netId;
	uint32 unk1;
	uint8 unk2;

	uint32 playerNo;
	ChatType type;
	uint32 lenght;
	uint8 unk3[32];
	int8 msg;

	int8 *getMessage()
	{
		return &msg;
	}
};

typedef struct _StatePacket
{
	_StatePacket(PacketCmd state)
	{
		header.cmd = state;
	}
	PacketHeader header;
} StatePacket;
typedef struct _StatePacket2
{
	_StatePacket2(PacketCmd state)
	{
		header.cmd = state;
		nUnk = 0;
	}
	PacketHeader header;
	int16 nUnk;
} StatePacket2;

struct FogUpdate2
{
	FogUpdate2()
	{
		header.cmd = PKT_S2C_FogUpdate2;
		header.netId = 0x40000019;
	}
	PacketHeader header;
	float x;
	float y;
	uint32 radius;
	uint8 unk1;
};

struct HeroSpawn
{
	HeroSpawn()
	{
		header.cmd = PKT_S2C_HeroSpawn;
		memset(&name, 0, 128+40); //Set name + type to zero
	}

	PacketHeader header;//5
	uint32 netId;//9
	uint32 playerId;//13
	uint8 nodeId;//14
	uint8 skillLevel;//15
	uint8 teamIsOrder;//16
	uint8 isBot;//17
//	uint8 botRank;//18
	uint8 spawnPos;//19
	uint32 skinNo;//23
	char name[128];
	char type[40];
	float deathDurationRemaining;
	float timeSinceDeath;
	uint8 bitField;
} ;
struct HeroSpawn2 {
	HeroSpawn2() {
		header.cmd = (PacketCmd)0xB9;
		ZeroMemory(unk, 30);
		unk[15] = 0x80;
		unk[16] = 0x3F;
		unk1 = 3;
		unk2 = 1;
		f1 = 0x420F9C78;
		f2 = 0x4388C6A5;
		f3 = 0x3F441B7D;
		f4 = 0x3F248DBB;
	}

	PacketHeader header;
	uint8 unk[30];
	uint8 unk1;
	uint32 unk2;
	DWORD f1;
	DWORD f2;
	DWORD f3;
	DWORD f4;
};
struct HeroSpawn3 {
	HeroSpawn3() {
		header.cmd = (PacketCmd)0xAD;
		unk = 0;
		health = 1337;
		maxHealth = 666;
	}

	PacketHeader header;
	uint16 unk;
	float health;
	float maxHealth;
};

struct TurretSpawn
{
	TurretSpawn()
	{
		header.cmd = PKT_S2C_TurretSpawn;
		tID = 0;
		unk = 0;
		memset(&name, 0, 29 + 42); //Set name + type to zero

	}

	PacketHeader header;
	uint8 tID;
	uint16 unk;
	uint8 name[29];
	uint8 type[42];
};
struct GameTimer {
    GameTimer(float fTime) {
        header.cmd = PKT_S2C_GameTimer;
        header.netId = 0;
        this->fTime = fTime;
    }
    PacketHeader header;
    float fTime;
};
struct GameTimerUpdate {
    GameTimerUpdate(float fTime) {
        header.cmd = PKT_S2C_GameTimerUpdate;
        header.netId = 0;
        this->fTime = fTime;
    }
    PacketHeader header;
    float fTime;
};
struct SpellSet {
    SpellSet(DWORD netID, uint32 _spellID, uint32 _level) {
        ZeroMemory(this, sizeof(SpellSet));
        header.cmd = PacketCmd(0x5A);
        header.netId = netID;
        spellID = _spellID;
        level = _level;
    }
    PacketHeader header;
    uint32 spellID;
    uint32 level;
};
class LevelPropSpawn
{
public:
	LevelPropSpawn()
	{
		header.cmd = PKT_S2C_LevelPropSpawn;
		netId = 0;
		memset(&unk, 0, 50 + 64 + 64); //Set name + type to zero
	}
	void SetProp(char* szName, char* szType) {
		header.cmd = PKT_S2C_LevelPropSpawn;
		netId = 0;
		x = 0;
		y = 0;
		memset(&unk, 0, 50);
		memset(&name, 0, 64);
		memset(&type, 0, 64);
		if (szName)
			strcpy((char*)name, szName);
		if (szType)
			strcpy((char*)type, szType);
	}
	PacketHeader header;
	uint32 netId;
	float x;
	float y;
	uint8 unk[50];
	uint8 name[64];
	uint8 type[64];
};

struct Announce
{
	PacketHeader header;
	uint8 msg;
	uint64 unknown;
	uint32 mapNo;
};

typedef struct _SkillUpPacket
{
	PacketHeader header;
	uint8 skill;
} SkillUpPacket;

typedef struct _SkillUpResponse
{
	_SkillUpResponse()
	{
		header.cmd = PKT_S2C_SkillUp;
		skill = 0;
		level = 0;
		pointsLeft = 0;
	}
	PacketHeader header;
	uint8 skill;
	uint8 level; //?
	uint8 pointsLeft;

} SkillUpResponse;

typedef struct _BuyItemReq
{
	PacketHeader header;
	uint32 id;
} BuyItemReq;

typedef struct _BuyItemAns
{
	_BuyItemAns()
	{
		header.cmd = (PacketCmd)PKT_S2C_BuyItemAns;
		unk1 = 0;
		unk2 = 0;
		unk3 = 0;
	}
	PacketHeader header;
	uint16 itemId;
	uint16 unk1;
	uint8 slotId;
	uint8 stack;
	uint16 unk2;
	uint8 unk3;
} BuyItemAns;

typedef struct _EmotionPacket
{
	PacketHeader header;
	uint8 id;
} EmotionPacket;

typedef struct _EmotionResponse
{
	_EmotionResponse()
	{
		header.cmd = PKT_S2C_Emotion;
	}
	PacketHeader header;
	uint8 id;
} EmotionResponse;
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

#endif