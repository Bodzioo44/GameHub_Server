#include "Server.h"


Server::Server()
{
    clients = {};
    running = true;
    lobby_id = 0;

    CreateSocket();
    Listening();
}

//TODO: All api request validity checks that can be done inside lobby should be done there.
void Server::HandleMessage(int clientSocket)
{
    char buffer[1024] = { 0 };
    //TODO: Increase buffer? 
    //multiple lobbies and game history can be quite long.
    ssize_t numbytes = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (numbytes == 0) {DisconnectClient(clientSocket);}
    else 
    {
        json message = json::parse(buffer);
        cout << message.dump(4) << endl;
        for (json::iterator it = message.begin(); it != message.end(); ++it)
        {
            //TODO Create string/funcion map?
            cout << "currently processing API: " << it.key() << endl;
            string current_api = it.key();

            if (current_api == API::ASSIGN_NAME)
            {
                cout << "Assigning name" << endl;
                if (Check_Name_Availablity(it.value()))
                {
                    Player* p = new Player(it.value(), clientSocket);
                    player_map[clientSocket] = p;
                    json response;
                    response[API::MESSAGE].push_back("You have joined the server as " + (std::string)it.value());
                    Send(clientSocket, response);
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
                //SAFETY TODO: Add check if player is already in a lobby.
                Lobby* new_lobby = new Lobby(Generate_Lobby_ID(), Get_GameType(it.value()), player_map[clientSocket]);
                lobby_map[new_lobby->Get_Lobby_ID()] = new_lobby;
                json response;
                response[API::MESSAGE].push_back("Lobby created with id: " + std::to_string(new_lobby->Get_Lobby_ID()));
                response[API::CREATE_LOBBY] = new_lobby->Get_Lobby_Info();
                Send(clientSocket, response);
            }

            //Check if Lobby exists.
            else if (current_api == API::JOIN_LOBBY)
            {
                int lobby_id = it.value();
                if (lobby_map.find(lobby_id) != lobby_map.end())
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
                Lobby* l = player_map[clientSocket]->Get_Lobby_ptr();
                //Add check if correct lobby is requested.
                //or dont send data at all, and just get the lobby id from the player. (YAP)
                Player* ppp = player_map[clientSocket];
                json response = l->Remove_Player(ppp);

                if (l->Get_Player_Count() < 1)
                {
                    lobby_map.erase(l->Get_Lobby_ID());
                    delete l;
                }   
                for (json::iterator it = response.begin(); it != response.end(); ++ it)
                {
                    Send(stoi(it.key()), it.value());
                }
            }
            else if (current_api == API::START_LOBBY)
            {
                Lobby* l = player_map[clientSocket]->Get_Lobby_ptr();
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
                Lobby* l = player_map[clientSocket]->Get_Lobby_ptr();
                json response = l->Game_Update(player_map[clientSocket], it.value());
                for (json::iterator it = response.begin(); it != response.end(); ++it)
                {
                    Send(stoi(it.key()), it.value());
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
        for (int clientSocket : clients)
        {
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
                if (input == "exit")
                {
                    running = false;
                }
                cout << input << endl;
            }
            else
            {
                for (int clientSocket : clients)
                {
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
    cout << "Client connected with fd: " << clientSocket << endl;
    clients.push_back(clientSocket);
}

void Server::DisconnectClient(int clientSocket)
{
    close(clientSocket);
    cout << "Client with fd: " << clientSocket << " disconnected" << endl;
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
}


void Server::Send(int client, json message)
{
    string serialized_message = message.dump();
    cout << "Sending serialized message: " << serialized_message << "With legth of: " << std::size(serialized_message) << endl;
    send(client, serialized_message.c_str(), serialized_message.size(), 0);

}

bool Server::Check_Name_Availablity(string name)
{
    for (auto const& pair : player_map)
    {
        if (pair.second->Get_Name() == name)
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

GameType Server::Get_GameType(int type)
{
    switch (type)
    {
        case 0:
            return GameType::CHECKERS_2;
        case 1:
            return GameType::CHESS_2;
        case 2:
            return GameType::CHESS_4;
        default:
            return GameType::CHESS_2;
    }
}


int main()
{
    Server server;
    return 0;
}