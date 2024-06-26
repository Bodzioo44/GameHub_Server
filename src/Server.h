#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <arpa/inet.h>

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "Lobby.h"
#include "Player.h"

#include "json.hpp"
#include "namespace.h"

using json = nlohmann::json;
using GameType = Enums::GameType;

const int PORT = 4444;
timeval timeout = {5, 0};


#include <iostream>
using namespace std;

class Server
{
public:
    Server();

private:
    void Listening();
    void CreateSocket();
    void HandleMessage(int clientSocket);
    void Send(int client, json message);
    void AssignClient();
    void DisconnectClient(int clientSocket);
    void ReconnectClient(int clientSocket);
    bool Check_Name_Availablity(string name);

    //Move these somewhere else?
    //util header file?
    int Generate_Lobby_ID();

    int serverSocket; //server socket
    int lobby_id;
    bool running;
    sockaddr_in serverAddress; //server addr

    map<int, Player*> player_map;
    map<int, Lobby*> lobby_map;
    map<string, Player*> disconnected_players;
    vector<int> Fds_to_remove;


};


#endif