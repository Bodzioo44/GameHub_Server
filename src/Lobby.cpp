#include "Lobby.h"

map<GameType, int> GameType_vals = 
{
    {GameType::CHECKERS_2, 2},
    {GameType::CHESS_2, 2},
    {GameType::CHESS_4, 4}
};

map<GameType, string> GameType_str = 
{
    {GameType::CHECKERS_2, "Checkers 2"},
    {GameType::CHESS_2, "Chess 2"},
    {GameType::CHESS_4, "Chess 4"}
};

map<Color, string> Coloruu = Color_str;


Lobby::Lobby(int lobby_id, GameType type, Player* p): lobby_id(lobby_id), type(type)
{
    max_player_count = GameType_vals[type];
    Live = false;
    player_count = 1;
    players.push_back(p);
    host = p;
    p->Join_Lobby(this);
    p->Set_Color(Assign_Color());

    cout << "Creating lobby with id: " << lobby_id << endl;
}

//each lobby method should return Player name alongside with hey/value pair (api_call, data).
// {
//     "PlayerName": 
//     {
//         "Message" : ["msg1", "msg2", "msg3"],
//         "Game_Update": "data",
//         ...
//     }
// }

json Lobby::Add_Player(Player* p)
{
    json Return_JSON;

    if (player_count < max_player_count && !Live)
    {   
        if (p->In_Lobby())
        {
            Return_JSON[p->Get_str_FD()][API::MESSAGE].push_back("You are already in a lobby.");
            return Return_JSON;
        }
        for (Player* pp: players)
        {
            Return_JSON[pp->Get_str_FD()][API::MESSAGE] = json::array();
            Return_JSON[pp->Get_str_FD()][API::MESSAGE].push_back(p->Get_Name() + " has joined the lobby.");
        }

        player_count++;
        players.push_back(p);
        p->Join_Lobby(this);
        p->Set_Color(Assign_Color());

        for (Player* pp: players)
        {
            Return_JSON[pp->Get_str_FD()][API::UPDATE_LOBBY] = Get_Lobby_Info();
        }

        Return_JSON[p->Get_str_FD()][API::MESSAGE] = json::array();
        Return_JSON[p->Get_str_FD()][API::MESSAGE].push_back("You have joined the lobby.");
        Return_JSON[p->Get_str_FD()][API::JOIN_LOBBY] = Get_Lobby_Info();
    }
    else if (!Live)
    {
        Return_JSON[p->Get_str_FD()][API::MESSAGE].push_back("Lobby is full.");
    }
    else
    {
        Return_JSON[p->Get_str_FD()][API::MESSAGE].push_back("Game has already started.");
    }
    return Return_JSON;
}


json Lobby::Remove_Player(Player* p)
{
    json response;
    cout << "we rae inside" << endl;
    if (!Live && player_count > 1)
    {
        player_count--;
        players.erase(std::remove(players.begin(), players.end(), p), players.end());
        p->Leave_Lobby();

        if (p == host)
        {
            host = players[0];
        }

        for (Player* pp: players)
        {
            response[pp->Get_str_FD()][API::MESSAGE].push_back(p->Get_Name() + " has left the lobby.");
            response[pp->Get_str_FD()][API::UPDATE_LOBBY] = Get_Lobby_Info();
        }
        response[p->Get_str_FD()][API::MESSAGE].push_back("You have left the lobby.");
        response[p->Get_str_FD()][API::LEAVE_LOBBY] = "0";
    }
    else if (Live)
    {
        response[p->Get_str_FD()][API::MESSAGE].push_back("Cant leave live game.");
    }
    else
    {
        response[p->Get_str_FD()][API::MESSAGE].push_back("You have left the lobby.");
        response[p->Get_str_FD()][API::LEAVE_LOBBY] = "0";
        player_count--;
        players.erase(std::remove(players.begin(), players.end(), p), players.end());
        p->Leave_Lobby();
    }
    return response;
}

json Lobby::Start_Lobby(Player* p)
{
    json response;
    if (p == host && player_count == max_player_count)
    {
        Live = true;
        for (Player* pp: players)
        {
            response[pp->Get_str_FD()][API::MESSAGE].push_back("Game has started.");
            response[pp->Get_str_FD()][API::START_LOBBY] = pp->Get_Color();
        }
    }
    else if (p != host)
    {
        response[p->Get_str_FD()][API::MESSAGE].push_back("Only the host can start the game.");
    }
    else
    {
        response[p->Get_str_FD()][API::MESSAGE].push_back("Not enough players.");
    }
    return response;
}


json Lobby::Game_Update(Player* p, json data)
{
    json response;
    for (Player* pp: players)
    {
        if (pp != p)
        {
            response[pp->Get_str_FD()][API::GAME_UPDATE] = data;
        }
    }
    return response;
}


json Lobby::Get_Lobby_Info() const
{   
    json response;

    response["Lobby_ID"] = lobby_id;
    response["Slots"] = max_player_count;
    response["GameType"] = GameType_str[type];
    response["Live"] = Live;

    response["Players"] = json::object();

    for (Player* p: players)
    {
        //TODO: replace with p->Get_Info()?
        response["Players"][p->Get_Name()].push_back(p->Get_Name());
        response["Players"][p->Get_Name()].push_back(Coloruu[p->Get_Color()]);
    }
    return response;
}


int Lobby::Get_Lobby_ID() const
{
    return lobby_id;
}



int Lobby::Get_Player_Count() const
{
    return player_count;
}


Lobby::~Lobby()
{
    std::cout << "Obliterated lobby with id: " << lobby_id << endl;
}


void Lobby::print_Lobby_Info() const
{
    cout << "Lobby ID: " << lobby_id << endl;
    cout << "Game Type: " << GameType_str[type] << endl;
    cout << "Player Count: " << player_count << endl;
    cout << "Host: " << host->Get_Name() << endl;
    cout << "Players: " << endl;
    for (Player* p: players)
    {
        cout << p->Get_Name() << endl;
    }
}

//FIXME: Ugly, but will work for testing.
Color Lobby::Assign_Color()
{
    if (player_count == 1)
    {
        return Color::WHITE;
    }
    else if (player_count == 2)
    {
        return Color::BLACK;
    }
    else if (player_count == 3)
    {
        return Color::ORANGE;
    }
    else
    {
        return Color::BLUE;
    }
}