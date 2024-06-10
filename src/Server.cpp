#include "Server.h"


Server::Server()
{
    //clients = {};
    running = true;
    lobby_id = 0;

    CreateSocket();
    Listening();
}

//TODO: All api request validity checks that can be done inside lobby should be done there.
void Server::HandleMessage(int clientSocket)
{
    char buffer[1024] = { 0 };
    ssize_t numbytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (numbytes == 0) {DisconnectClient(clientSocket);}

    else 
    {
        json message = json::parse(buffer);
        cout << message.dump(4) << endl;
        for (json::iterator it = message.begin(); it != message.end(); ++it)
        {
            cout << "currently processing API: " << it.key() << endl;
            string current_api = it.key();
            if (current_api == API::ASSIGN_NAME)
            {
                auto is_dc = disconnected_players.find(it.value());
                if (is_dc != disconnected_players.end())
                {
                    Player* dc_p = is_dc->second;
                    dc_p->socket_fd = clientSocket;
                    player_map[clientSocket] = dc_p;
                    disconnected_players.erase(it.value());
                    json response;
                    response = dc_p->lobby_ptr->Player_Reconnected(dc_p);
                    for (json::iterator it = response.begin(); it != response.end(); ++it)
                    {
                        Send(stoi(it.key()), it.value());
                    }
                }
                else
                {
                    if (Check_Name_Availablity(it.value()))
                    {
                        Player* p = new Player(it.value(), clientSocket);
                        player_map[clientSocket] = p;
                        json response;
                        response[API::MESSAGE].push_back("You have joined the server as " + (std::string)it.value());
                        Send(clientSocket, response);
                        cout << "Player with name: " << p->Get_name() << " has joined the server with fd: " << clientSocket << endl;
                    }
                    else
                    {
                        json response;
                        response[API::MESSAGE].push_back("Name already taken");
                        Send(clientSocket, response);
                        DisconnectClient(clientSocket);
                    }
                }

            }
            //TODO: Send every player without a lobby updated list.
            else if (current_api == API::CREATE_LOBBY)
            {
                if (!player_map[clientSocket]->lobby_ptr) //Check if player is already in a lobby.
                {
                    Lobby* new_lobby = new Lobby(Generate_Lobby_ID(), static_cast<GameType>(it.value()), player_map[clientSocket]);
                    lobby_map[new_lobby->Get_Lobby_ID()] = new_lobby;
                    json response;
                    response[API::MESSAGE].push_back("Lobby created with id: " + std::to_string(new_lobby->Get_Lobby_ID()));
                    response[API::CREATE_LOBBY] = new_lobby->Get_Lobby_Info();
                    Send(clientSocket, response);
                    cout << "Lobby created with id: " << new_lobby->Get_Lobby_ID() << "by player: " << player_map[clientSocket]->Get_name() << endl;
                }
                else
                {
                    json response;
                    response[API::MESSAGE].push_back("You are already in a lobby.");
                    Send(clientSocket, response);
                }
            }

            else if (current_api == API::JOIN_LOBBY)
            {
                int lobby_id = it.value();
                if (lobby_map.find(lobby_id) != lobby_map.end()) //Check if lobby id is correct.
                {
                    json response = lobby_map[lobby_id]->Player_Joined(player_map[clientSocket]);
                    for (json::iterator it = response.begin(); it != response.end(); ++it)
                    {
                        Send(stoi(it.key()), it.value());
                    }
                }
                else
                {
                    json response;
                    response[API::MESSAGE].push_back("Lobby does not exist.");
                    Send(clientSocket, response);
                }
            }
            else if (current_api == API::LEAVE_LOBBY)
            {
                Player* current_player = player_map[clientSocket];
                Lobby* current_lobby = current_player->lobby_ptr;


                json response = current_lobby->Player_Left(current_player);

                if (current_lobby->Is_Empty())
                {
                    lobby_map.erase(current_lobby->Get_Lobby_ID());
                    //TODO: before deleting the lobby remove all disconnected players (inside lobby destructor?)
                    //there will be no disconnected players inside non live lobby??
                    //separate api for whenever player leaves live lobby? (outside of disconnects)
                    delete current_lobby;
                }
                
                for (json::iterator it = response.begin(); it != response.end(); ++ it)
                {
                    Send(stoi(it.key()), it.value());
                }
            }
            else if (current_api == API::START_LOBBY)
            {
                Lobby* l = player_map[clientSocket]->lobby_ptr;
                json response = l->Start_Lobby(player_map[clientSocket]);
                for (json::iterator it = response.begin(); it != response.end(); ++it)
                {
                    Send(stoi(it.key()), it.value());
                }
            }

            //TODO: Replace with UPDATE_LOBBY_LIST? REQUEST_LOBBY_LIST SEEMS OBSOLETE.
            else if (current_api == API::REQUEST_LOBBY_LIST)
            //Add check if any lobby exists. to avoid sending null.
            {
                json response;
                response[API::UPDATE_LOBBY_LIST] = {};
                for (auto const& pair : lobby_map)
                {
                    response[API::UPDATE_LOBBY_LIST][std::to_string(pair.first)] = pair.second->Get_Lobby_Info();
                }

                Send(clientSocket, response);
            }
            else if (current_api == API::UPDATE_LOBBY_LIST)
            {
                cout << "Updating lobby list" << endl;
            }
            else if (current_api == API::GAME_UPDATE)
            {
                // json game_update = it.value();
                // for (json::iterator it = game_update.begin(); it != game_update.end(); ++it)
                // {
                //     cout << it.key() << " : " << it.value() << endl;
                // }
                Lobby* l = player_map[clientSocket]->lobby_ptr;
                json response = l->Game_Update(player_map[clientSocket], it.value());
                for (json::iterator it = response.begin(); it != response.end(); ++it)
                {
                    Send(stoi(it.key()), it.value());
                }
            }
            else if (current_api == API::GLOBAL_MESSAGE)
            {
                
                json response;
                Player* p = player_map[clientSocket];
                string message = it.value();
                response[API::GLOBAL_MESSAGE] = p->Get_name() + ": " + message;
                for (auto const& pair : player_map)
                {
                    Send(pair.first, response);
                }
            }
            else if (current_api == API::LOBBY_MESSAGE)
            {
                Player* current_player = player_map[clientSocket];
                Lobby* current_lobby = current_player->lobby_ptr;

                json response;
                if (current_lobby)
                {
                    response = current_lobby->Send_Lobby_Message(it.value());
                    for (json::iterator it = response.begin(); it != response.end(); ++it)
                    {
                        Send(stoi(it.key()), it.value());
                    }
                }
                else
                {
                    response[API::MESSAGE].push_back("You are not in a lobby.");
                    Send(clientSocket, response);
                }
            }


            
            else
            {
                cout << "Unknown API" << endl;
            }            
        }
    }
}


void Server::CreateSocket()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(PORT); 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
    int yes = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)); 
}

void Server::Listening()
{
    listen(serverSocket, SOMAXCONN); 
    while (running)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_sock_value = serverSocket;
        if (STDIN_FILENO > max_sock_value)
        {
            max_sock_value = STDIN_FILENO;
        }
        for (auto player_pair : player_map)
        {
            int clientSocket = player_pair.first;
            FD_SET(clientSocket, &readfds);
            if (clientSocket > max_sock_value)
            {
                max_sock_value = clientSocket;
            }
        }
        timeout.tv_sec = 3;
        int activity = select(max_sock_value + 1, &readfds, nullptr, nullptr, &timeout);

        if (activity > 0)
        {
            if (FD_ISSET(serverSocket, &readfds))
            {
                AssignClient();
            }
            else if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                string input;
                getline(cin, input);
                if (input == "stop")
                {
                    running = false;
                }
                else if (input == "dc")
                {
                    cout << "Disconnected Players: ";
                    for (auto pair : disconnected_players)  {cout << pair.first << ", ";}
                    cout << endl;
                }
                else if (input == "p")
                {
                    cout << "Players: ";
                    for (auto pair : player_map)  {cout << pair.second->Get_name() << ", ";}
                    cout << endl;
                }
                else if (input == "l")
                {
                    cout << "Lobby ID's: ";
                    for (auto pair : lobby_map)  {cout << pair.first << ", ";}
                    cout << endl;
                }
                else if (input == "info")
                {
                    cout << "All lobby info: " << endl;
                    for (auto pair : lobby_map)
                    {
                        pair.second->Print_Lobby_Info();
                    }
                }

                //cout << input << endl;
            }
            else
            {
                for (auto player_pair : player_map)
                {
                    int clientSocket = player_pair.first;
                    if (FD_ISSET(clientSocket, &readfds))
                    {    
                        HandleMessage(clientSocket);
                    }
                }
                for (int fd : Fds_to_remove)
                {
                    player_map.erase(fd);
                }
                Fds_to_remove.clear();
            }
        }

        else if (activity < 0)
        {
            cout << "Select error, something went really wrong!" << endl;
            perror("select");
        }
        else
        {
            //cout << "Timeoutaaa!" << endl;
        }
    }
    close(serverSocket); 
}

void Server::AssignClient()
{
    // TODO add error handling, store client addresses?
    // sockaddr_in clientAddress;
    // socklen_t clientSize = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    cout << "New connection with fd: " << clientSocket << endl;
    //clients.push_back(clientSocket);
    player_map[clientSocket] = nullptr;
}

void Server::ReconnectClient(int clientSocket)
{

}

//Player* ptrs are stored inside player_map, and inside lobby.
//Whenever player disconnects, player is always removed from player_map.
//Lobby class can decide whether to keep (for future reconnects) or delete the player object through pointer.

void Server::DisconnectClient(int clientSocket)
{
    close(clientSocket);
    cout << "Client with fd: " << clientSocket << " has been disconnected" << endl;
    //clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());

    if (player_map[clientSocket] != nullptr) //check if fd had assigned player.
    {
        Player* current_player = player_map[clientSocket];
        Lobby* current_lobby = current_player->lobby_ptr;
        if (current_lobby)
        {
            json response = current_lobby->Player_Disconnected(current_player);
            if (current_lobby->Is_Empty())
            {
                lobby_map.erase(current_lobby->Get_Lobby_ID());
                for (Player* p : current_lobby->Get_DC_Players())
                {
                    disconnected_players.erase(p->Get_name());
                }
                delete current_lobby;
            }
            else
            {
                if (response.find("Disconnect_Player") != response.end())
                {
                    Player* player_to_dc = player_map[(response["Disconnect_Player"])];
                    cout << "Player to disconnect (moved inside dc_player vector): " << player_to_dc->Get_name() << endl;
                    disconnected_players[player_to_dc->Get_name()] = player_to_dc;
                    response.erase("Disconnect_Player");
                }
                

                for (json::iterator it = response.begin(); it != response.end(); ++it)
                {
                    Send(stoi(it.key()), it.value());
                }
            }
        }
        else {delete current_player;}
    }
    //player_map.erase(clientSocket); //erasing stuff from map while iterating over it is VERY BAD.
    Fds_to_remove.push_back(clientSocket);

}

        // if (current_lobby && current_lobby->Is_Live()) //check if player is in a lobby.
        // {

        //     json response = current_lobby->Player_Disconnected(player);
        //     if (current_lobby->Is_Empty()) //check if lobby is empty after player disconnect.
        //     {
        //         lobby_map.erase(current_lobby->Get_Lobby_ID());
        //         //TODO: before deleting the lobby remove all disconnected players.
        //         //has to be outside of lobby destructor to also remove map entries?
        //         delete current_lobby;
        //     }
        //     else //if there are still players in the lobby, send the update to them.
        //     {
        //         for (json::iterator it = response.begin(); it != response.end(); ++it)
        //         {
        //             Send(stoi(it.key()), it.value());
        //         }
        //     }
        // }
        // //delete player_map[clientSocket];
    

void Server::Send(int client, json message)
{
    string serialized_message = message.dump();
    cout << "Sending serialized message With legth of " << std::size(serialized_message) << " to FD " << client << ": " << serialized_message << endl;
    send(client, serialized_message.c_str(), serialized_message.size(), 0);

}

bool Server::Check_Name_Availablity(string name)
{
    for (auto pair : player_map)
    {
        if (pair.second != nullptr && pair.second->Get_name() == name)
        {
            return false;
        }
    }
    return true;
}

int Server::Generate_Lobby_ID()
{
    //TODO: Reuse lobby ids.
    return lobby_id++;
}


int main()
{
    Server server;
    return 0;
}