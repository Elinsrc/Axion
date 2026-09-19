#pragma once

extern int blue_flag_player_index;
extern int red_flag_player_index;

class CHudCTF : public CHudBase
{
	int flag_sprite_indices[4];
	HLSPRITE flag_sprites[4];

	enum class FlagState {
		HOME = 0,
		STOLEN = 1,
		LOST = 2,
		CARRY = 3
	};

	FlagState blue_flag_state;
	FlagState red_flag_state;

public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw(float time);

	inline int GetBlueFlagPlayerIndex() const
	{
		return blue_flag_player_index;
	}

	inline int GetRedFlagPlayerIndex() const
	{
		return red_flag_player_index;
	}

	int MsgFunc_CTF(const char* name, int size, void* buf);
	int MsgFunc_CTFSound(const char* name, int size, void* buf);
	int MsgFunc_CTFFlag(const char* name, int size, void* buf);
};
