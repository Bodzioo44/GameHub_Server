#ifndef PLAYER_H
#define PLAYER_H

#include <string>
using std::string;

#include "enums.h"
#include "json.hpp"

using namespace Enums;
using json = nlohmann::json;

using namespace std;
#include <iostream>

class Lobby;


class Player
{
    public:
        Player(string name, int socket_fd);
        ~Player();

        Color color;
        int socket_fd;
        Lobby* lobby_ptr;

        string Get_name() const;
        json Get_player_info() const;



    private:

        string name;
};


#endif