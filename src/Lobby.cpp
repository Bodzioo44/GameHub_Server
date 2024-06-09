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

Lobby::Lobby(int lobby_id, GameType type, Player* p): lobby_id(lobby_id), type(type)
{
    max_player_count = GameType_vals[type];
    Live = false;
    Lobby_Empty = false;
    player_count = 0;
    host = p;
    add_player(p);

    cout << "Creating lobby with id: " << lobby_id << endl;
}


json Lobby::Player_Joined(Player* p)
{
    json Return_JSON;
    if (player_count < max_player_count && !Live)
    {   
        if (p->lobby_ptr)
        {
            Return_JSON[to_string(p->socket_fd)][API::MESSAGE].push_back("You are already in a lobby.");
            return Return_JSON;
        }
        for (Player* pp: players)
        {
            Return_JSON[to_string(pp->socket_fd)][API::MESSAGE].push_back(p->Get_name() + " has joined the lobby.");
        }

        add_player(p);
        //FIXME: Sending the player same info with UPDATE_LOBBY and JOIN_LOBBY?
        for (Player* pp: players)
        {
            Return_JSON[to_string(pp->socket_fd)][API::UPDATE_LOBBY] = Get_Lobby_Info();
        }
        Return_JSON[to_string(p->socket_fd)][API::MESSAGE].push_back("You have joined the lobby.");
        Return_JSON[to_string(p->socket_fd)][API::JOIN_LOBBY] = Get_Lobby_Info();
    }
    else if (!Live)
    {
        Return_JSON[to_string(p->socket_fd)][API::MESSAGE].push_back("Lobby is full.");
    }
    else
    {
        Return_JSON[to_string(p->socket_fd)][API::MESSAGE].push_back("Game has already started.");
    }
    return Return_JSON;
}

json Lobby::Player_Left(Player* p)
{
    json response;
    if (!Live && player_count > 1)
    {
        remove_player(p);
        if (p == host)
        {
            host = players[0];
        }

        for (Player* pp: players)
        {
            response[to_string(pp->socket_fd)][API::MESSAGE].push_back(p->Get_name() + " has left the lobby.");
            response[to_string(pp->socket_fd)][API::UPDATE_LOBBY] = Get_Lobby_Info();
        }
        response[to_string(p->socket_fd)][API::MESSAGE].push_back("You have left the lobby.");
        response[to_string(p->socket_fd)][API::LEAVE_LOBBY] = "0";
    }
    else if (!Live && player_count == 1)
    {
        response[to_string(p->socket_fd)][API::MESSAGE].push_back("You have left the lobby.");
        response[to_string(p->socket_fd)][API::LEAVE_LOBBY] = "0";
        remove_player(p);
        Lobby_Empty = true;
    }
    else if (Live)
    {
        response[to_string(p->socket_fd)][API::MESSAGE].push_back("Cant leave live game.");
    }
    return response;
}

//Ugly af
json Lobby::Player_Disconnected(Player* p)
{
    json response;
    //cout << "Player disconnected: " << p->Get_name();
    if (!Live && player_count > 1) //Game isnt live yet, just delete the player and let others know that player has disconnected.
    {
        //cout << " But the game wasnt live!" << endl;
        remove_player(p);
        if (p == host)
        {
            host = players[0];
        }
        for (Player* pp: players)
        {
            response[to_string(pp->socket_fd)][API::MESSAGE].push_back(p->Get_name() + " has disconnected.");
            response[to_string(pp->socket_fd)][API::UPDATE_LOBBY] = Get_Lobby_Info();
        }
        delete p;
    }
    else if (!Live && player_count == 1) //Game isnt live yet and only one player left. delete the player and let server know that lobby is empty.
    {
        //cout << " But the game wasnt live and only one player left!" << endl;
        remove_player(p);
        Lobby_Empty = true;
        delete p;
    }
    else if (Live && player_count > 1) //Game is live. move player to disconnected_players and let others know that player has disconnected.
    {
        //cout << " And the game was live!";
        for (Player* pp: players)
        {
            response[to_string(pp->socket_fd)][API::MESSAGE].push_back(p->Get_name() + " has disconnected.");
        }
        disconnected_players.push_back(p);
        response["Disconnect_Player"] = p->socket_fd;
        remove_player(p);
        p->lobby_ptr = this;

    }
    //Game is live and only one player left. move player to disconnected_players and let server know that lobby is empty.
    //Server will delete the lobby, and through the destructor, also delete the disconnected players.
    else if (Live && player_count == 1)
    {
        //cout << " And the game was live and only one player left!";
        disconnected_players.push_back(p);
        remove_player(p);
        Lobby_Empty = true;
    }
    // for (Player* p : disconnected_players)
    // {
    //     cout << "Disconnected players right before return: " << p->Get_name() << endl;
    // }
    return response;
}

json Lobby::Player_Reconnected(Player* p)
{
    json response;
    disconnected_players.erase(std::remove(disconnected_players.begin(), disconnected_players.end(), p), disconnected_players.end());
    for (Player* pp: players)
    {
        response[to_string(pp->socket_fd)][API::MESSAGE].push_back(p->Get_name() + " has reconnected.");
    }
    add_player(p);

    response[to_string(p->socket_fd)][API::MESSAGE].push_back("You have been reconnected to the server.");

    response[to_string(p->socket_fd)][API::RECONNECT][API::START_LOBBY] = p->color;
    response[to_string(p->socket_fd)][API::RECONNECT][API::GAME_HISTORY] = history;

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
            response[to_string(pp->socket_fd)][API::MESSAGE].push_back("Game has started.");
            response[to_string(pp->socket_fd)][API::START_LOBBY] = pp->color;
        }
    }
    else if (p != host)
    {
        response[to_string(p->socket_fd)][API::MESSAGE].push_back("Only the host can start the game.");
    }
    else
    {
        response[to_string(p->socket_fd)][API::MESSAGE].push_back("Not enough players.");
    }
    return response;
}


json Lobby::Game_Update(Player* p, json data)
{
    json response;
    history.push_back(data);
    for (Player* pp: players)
    {
        if (pp != p)
        {
            response[to_string(pp->socket_fd)][API::GAME_UPDATE] = data;
        }
    }
    return response;
}

json Lobby::Send_Lobby_Message(string message) const
{
    json response;
    for (Player* p: players)
    {
        response[to_string(p->socket_fd)][API::LOBBY_MESSAGE] = message;
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
        //TODO: replace with p->Get_Info()? send fd instead of name?
        response["Players"][p->Get_name()] = p->Get_player_info();
    }
    return response;
}

void Lobby::add_player(Player* p)
{
    player_count++;
    players.push_back(p);
    p->lobby_ptr = this;
    p->color = Assign_Color();
}

void Lobby::remove_player(Player* p)
{
    player_count--;
    players.erase(std::remove(players.begin(), players.end(), p), players.end());
    p->lobby_ptr = nullptr;
}

// Player* Lobby::disconnect_player(Player* p)
// {

// }

Lobby::~Lobby()
{
    std::cout << "Removed Lobby with id (inside deconstructor): " << lobby_id << endl;
    for (Player* p : disconnected_players)
    {
        std::cout << "Removed disconnected player: "  << p->Get_name() << endl;
        delete p;
    }
}


void Lobby::Print_Lobby_Info() const
{
    cout << endl << endl;
    cout << "Lobby ID: " << lobby_id << endl;
    cout << "Game Type: " << GameType_str[type] << endl;
    cout << "Player Count: " << player_count << endl;
    cout << "Host: " << host->Get_name() << endl;
    cout << "Live: " << (Live ? "Yes" : "No") << endl;
    cout << "Players: ";
    for (Player* p: players)
    {
        cout << p->Get_name() << ", ";
    }
    cout << endl;
    cout << "Disconnected Players: ";
    for (Player* p: disconnected_players)
    {
        cout << p->Get_name() << ", ";
    }
    cout << endl;
    cout << "History: ";
    for (json j: history)
    {
        cout << j << ", ";
    }
    cout << endl << endl;
}

int Lobby::Get_Lobby_ID() const
{
    return lobby_id;
}


int Lobby::Get_Player_Count() const
{
    return player_count;
}

vector<Player*> Lobby::Get_DC_Players() const
{
    return disconnected_players;
}

bool Lobby::Is_Empty() const
{
    return Lobby_Empty;
}

bool Lobby::Is_Live() const
{
    return Live;
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