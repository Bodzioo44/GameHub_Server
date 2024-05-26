#include "Player.h"

Player::Player(string name, int socket_fd): name(name), socket_fd(socket_fd)
{
    lobby_ptr = nullptr;
}

Player::~Player() {}


int Player::Get_FD() const
{
    return socket_fd;
}

string Player::Get_str_FD() const
{
    return std::to_string(socket_fd);
}

string Player::Get_Name() const
{
    return name;
}


void Player::Leave_Lobby()
{
    lobby_ptr = nullptr;
}

void Player::Join_Lobby(Lobby* l)
{
    lobby_ptr = l;
}

bool Player::In_Lobby() const
{
    return lobby_ptr != nullptr;
}

Lobby* Player::Get_Lobby_ptr() const
{
    return lobby_ptr;
}


void Player::Get_Info() const
{
    std::cout << "Player: " << name << " with fd: " << socket_fd << std::endl;
}


Color Player::Get_Color() const
{
    return color;
}

void Player::Set_Color(Color c)
{
    color = c;
}