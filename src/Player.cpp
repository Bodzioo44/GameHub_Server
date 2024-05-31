#include "Player.h"

Player::Player(string name, int socket_fd): name(name), socket_fd(socket_fd)
{
    lobby_ptr = nullptr;
}

Player::~Player() {}

string Player::Get_name() const
{
    return name;
}

json Player::Get_player_info() const
{
    json player_info;
    player_info.push_back(name);
    player_info.push_back(color);
    return player_info;
}