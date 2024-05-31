#ifndef LOBBY_H
#define LOBBY_H

#include "Player.h"
#include "json.hpp"
#include "enums.h"


#include <vector>
#include <map>
#include <algorithm>


using std::vector, std::map;
using json = nlohmann::json;
using GameType = Enums::GameType;

using namespace Enums;


#include <iostream>
using namespace std;



class Lobby
{
public:
    Lobby(int lobby_id, GameType type, Player* p);
    ~Lobby();

    int Get_Lobby_ID() const;
    
    json Get_Lobby_Info() const;

    void print_Lobby_Info() const;

    int Get_Player_Count() const;



    json Add_Player(Player* p);
    json Remove_Player(Player* p);
    json Start_Lobby(Player* p);
    json Game_Update(Player* p, json data);
    json Send_Lobby_Message(string message);

private:

    vector<Player*> players;

    Player* host;
    bool Live;
    GameType type;
    int unsigned player_count;
    int unsigned lobby_id;
    int unsigned max_player_count;

    Color Assign_Color();
    vector<json> history;

};

#endif