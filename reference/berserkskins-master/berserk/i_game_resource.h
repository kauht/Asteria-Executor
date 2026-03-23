#pragma once


class CGameEntitySystem;

class i_game_resource
{
public:
	char padding[0x58];
	CGameEntitySystem* pGameEntitySystem;
};