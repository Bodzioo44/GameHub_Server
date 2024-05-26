#ifndef PLAYER_H
#define PLAYER_H

#include <string>

using std::string;
#include <iostream>
#include "enums.h"
using namespace Enums;

class Lobby;


class Player
{
    public:
        Player(string name, int socket_fd);
        ~Player();
        int Get_FD() const;
        string Get_str_FD() const;
        string Get_Name() const;
        Lobby* Get_Lobby_ptr() const;
        void Get_Info() const;
        Color Get_Color() const;

        void Set_Color(Color);
        

        void Join_Lobby(Lobby* l);
        void Leave_Lobby();
        bool In_Lobby() const;

    private:
        Lobby* lobby_ptr;
        int socket_fd;
        string name;
        Color color;
        // lobby id inside player?
};


#endif