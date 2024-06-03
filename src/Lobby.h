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

    json Player_Joined(Player* p);
    json Player_Left(Player* p);
    json Player_Disconnected(Player* p);

    json Start_Lobby(Player* p);
    json Game_Update(Player* p, json data) const;
    json Send_Lobby_Message(string message) const; //sends message to all players in lobby
    json Get_Lobby_Info() const; //returns lobby info in json, used for sending lobby info to clients

    int Get_Lobby_ID() const; //returns lobby id
    int Get_Player_Count() const; //returns player count
    void Print_Lobby_Info() const; //prints lobby info to console
    bool Is_Empty() const; //returns true if lobby is empty
    bool Is_Live() const; //returns true if lobby is live

private:
    void add_player(Player*);
    void remove_player(Player*);

    vector<Player*> players;
    vector<Player*> disconnected_players;
    Player* host;
    bool Live;
    bool Lobby_Empty;
    GameType type;
    int unsigned player_count;
    int unsigned lobby_id;
    int unsigned max_player_count;

    Color Assign_Color();
    vector<json> history;

};

#endif