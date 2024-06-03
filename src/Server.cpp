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
                    json response = lobby_map[lobby_id]->Add_Player(player_map[clientSocket]);
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


                json response = current_lobby->Remove_Player(current_player);

                if (response["Remove_Lobby"])
                {
                    lobby_map.erase(current_lobby->Get_Lobby_ID());
                    //TODO: before deleting the lobby remove all disconnected players.
                    delete current_lobby;
                }
                response.erase("Remove_Lobby");
                
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
                response[API::GLOBAL_MESSAGE] = it.value();
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
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, nullptr, 0);
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
            }
        }

        else if (activity < 0)
        {
            cout << "Select error, something went wrong!" << endl;
            perror("select");
        }
        else
        {
            //cout << "Timeouaaa!" << endl;
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

void Server::DisconnectClient(int clientSocket)
{
    close(clientSocket);
    cout << "Client with fd: " << clientSocket << " disconnected" << endl;
    //clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());

    if (player_map[clientSocket] != nullptr) // 
    {
        Player* player = player_map[clientSocket];
        Lobby* current_lobby = player->lobby_ptr;
        if (player->lobby_ptr)
        {
            json response = current_lobby->Remove_Player(player);

            if (response["Remove_Lobby"])
            {
                lobby_map.erase(current_lobby->Get_Lobby_ID());
                //TODO: before deleting the lobby remove all disconnected players.
                delete current_lobby;
            }

            for (json::iterator it = response.begin(); it != response.end(); ++it)
            {
                Send(stoi(it.key()), it.value());
            }
        }
        //delete player_map[clientSocket];
    }
    player_map.erase(clientSocket);
}


void Server::Send(int client, json message)
{
    string serialized_message = message.dump();
    cout << "Sending serialized message: " << serialized_message << "With legth of: " << std::size(serialized_message) << endl;
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