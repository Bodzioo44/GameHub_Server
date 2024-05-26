#ifndef ENUMS_H
#define ENUMS_H

#include <map>
#include <string>

using std::map, std::string;

namespace Enums
{
    enum class Color
    {
        WHITE,
        BLACK,
        ORANGE,
        BLUE
    };

    enum class GameType
    {
        CHECKERS_2,
        CHESS_2,
        CHESS_4
    };

    enum class SquareState 
    {
        INVALID,
        EMPTY,
        TAKEN_BY_ENEMY,
        TAKEN_BY_FRIENDLY
    };

    const map<Enums::Color, string> Color_str = 
    {
        {Enums::Color::WHITE, "White"},
        {Enums::Color::BLACK, "Black"},
        {Enums::Color::ORANGE, "Orange"},
        {Enums::Color::BLUE, "Blue"}
    };

}

//Assigning constants inside a header file is ok?
namespace API
{
    const string ASSIGN_NAME = "Assign_Name";
    const string JOIN_LOBBY = "Join_Lobby";
    const string CREATE_LOBBY = "Create_Lobby";
    const string LEAVE_LOBBY = "Leave_Lobby";
    const string REQUEST_LOBBY_LIST = "Request_Lobby_List";
    const string UPDATE_LOBBY_LIST = "Update_Lobby_List";
    const string GAME_UPDATE = "Game_Update";
    const string MESSAGE = "Message";
    const string DISCONNECT = "Disconnect";
    const string UPDATE_LOBBY = "Update_Lobby";
    const string START_LOBBY = "Start_Lobby";
}


#endif