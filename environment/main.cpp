#include <iostream>
#include <list>

#include "core/Halite.hpp"

inline std::istream& operator>>(std::istream& i,
                                std::pair<signed int, signed int>& p) {
    i >> p.first >> p.second;
    return i;
}

inline std::ostream& operator<<(std::ostream& o,
                                const std::pair<signed int, signed int>& p) {
    o << p.first << ' ' << p.second;
    return o;
}

#include <tclap/CmdLine.h>

namespace TCLAP {
    template<>
    struct ArgTraits<std::pair<signed int, signed int> > {
        typedef TCLAP::ValueLike ValueCategory;
    };
}

bool quiet_output =
    false; //Need to be passed to a bunch of classes; extern is cleaner.
Halite*
    my_game; //Is a pointer to avoid problems with assignment, dynamic memory, and default constructors.

Networking promptNetworking();
void promptDimensions(unsigned short& w, unsigned short& h);

int main(int argc, char** argv) {
    srand(time(NULL)); //For all non-seeded randomness.

    Networking networking;
    std::vector<std::string>* names = NULL;
    unsigned int id =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();

    TCLAP::CmdLine cmd("Halite Game Environment", ' ', "1.2");

    //Switch Args.
    TCLAP::SwitchArg quietSwitch("q",
                                 "quiet",
                                 "Runs game in quiet mode, producing machine-parsable output.",
                                 cmd,
                                 false);
    TCLAP::SwitchArg overrideSwitch("o",
                                    "override",
                                    "Overrides player-sent names using cmd args [SERVER ONLY].",
                                    cmd,
                                    false);
    TCLAP::SwitchArg timeoutSwitch("t",
                                   "timeout",
                                   "Causes game environment to ignore timeouts (give all bots infinite time).",
                                   cmd,
                                   false);
    TCLAP::SwitchArg noReplaySwitch
        ("r", "noreplay", "Turns off the replay generation.", cmd, false);

    //Value Args
    TCLAP::ValueArg<unsigned int> nPlayersArg("n",
                                              "nplayers",
                                              "Create a map that will accommodate n players [SINGLE PLAYER MODE ONLY].",
                                              false,
                                              1,
                                              "{1,2,3,4,5,6}",
                                              cmd);
    TCLAP::ValueArg<std::pair<signed int, signed int> > dimensionArgs("d",
                                                                      "dimensions",
                                                                      "The dimensions of the map.",
                                                                      false,
                                                                      { 0,
                                                                        0 },
                                                                      "a string containing two space-seprated positive integers",
                                                                      cmd);
    TCLAP::ValueArg<unsigned int> seedArg("s",
                                          "seed",
                                          "The seed for the map generator.",
                                          false,
                                          0,
                                          "positive integer",
                                          cmd);
    TCLAP::ValueArg<std::string> replayDirectoryArg("i",
                                                    "replaydirectory",
                                                    "The path to directory for replay output.",
                                                    false,
                                                    ".",
                                                    "path to directory",
                                                    cmd);
    TCLAP::ValueArg<std::string> constantsArg(
        "",
        "constantsfile",
        "JSON file containing runtime constants to use.",
        false,
        "",
        "path to file",
        cmd
    );

    TCLAP::SwitchArg printConstantsSwitch(
        "",
        "print-constants",
        "Print out the default constants and exit.",
        cmd,
        false
    );

    //Remaining Args, be they start commands and/or override names. Description only includes start commands since it will only be seen on local testing.
    TCLAP::UnlabeledMultiArg<std::string> otherArgs("NonspecifiedArgs",
                                                    "Start commands for bots.",
                                                    false,
                                                    "Array of strings",
                                                    cmd);

    cmd.parse(argc, argv);

    unsigned short mapWidth = dimensionArgs.getValue().first;
    unsigned short mapHeight = dimensionArgs.getValue().second;

    unsigned int seed;
    if (seedArg.getValue() != 0) seed = seedArg.getValue();
    else
        seed =
            (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()
                % 4294967295);

    unsigned short n_players_for_map_creation = nPlayersArg.getValue();

    quiet_output = quietSwitch.getValue();
    bool override_names = overrideSwitch.getValue();
    bool ignore_timeout = timeoutSwitch.getValue();

    if (printConstantsSwitch.getValue()) {
        std::cout << hlt::GameConstants::get().to_json().dump(4) << '\n';
        return 0;
    }

    // Update the game constants.
    if (constantsArg.isSet()) {
        std::ifstream constants_file(constantsArg.getValue());
        nlohmann::json constants_json;
        constants_file >> constants_json;
        auto& constants = hlt::GameConstants::get_mut();
        constants.from_json(constants_json);

        if (!quiet_output) {
            const auto& constants = hlt::GameConstants::get();
            std::cout
                << "Game constants: \n"
                << "\tPLANETS_PER_PLAYER: " << constants.PLANETS_PER_PLAYER << '\n'
                << "\tEXTRA_PLANETS: " << constants.EXTRA_PLANETS << '\n'

                << "\tDRAG: " << constants.DRAG << '\n'
                << "\tMAX_SPEED: " << constants.MAX_SPEED << '\n'
                << "\tMAX_ACCELERATION: " << constants.MAX_ACCELERATION << '\n'

                << "\tMAX_SHIP_HEALTH: " << constants.MAX_SHIP_HEALTH << '\n'
                << "\tBASE_SHIP_HEALTH: " << constants.BASE_SHIP_HEALTH << '\n'
                << "\tDOCKED_SHIP_REGENERATION: " << constants.DOCKED_SHIP_REGENERATION << '\n'

                << "\tWEAPON_COOLDOWN: " << constants.WEAPON_COOLDOWN << '\n'
                << "\tWEAPON_RADIUS: " << constants.WEAPON_RADIUS << '\n'
                << "\tWEAPON_DAMAGE: " << constants.WEAPON_DAMAGE << '\n'

                << "\tDOCK_TURNS: " << constants.DOCK_TURNS << '\n'
                << "\tPRODUCTION_PER_SHIP: " << constants.PRODUCTION_PER_SHIP << '\n'
                << "\tMAX_DOCKING_DISTANCE: " << constants.MAX_DOCKING_DISTANCE << '\n';
        }
    }
    else {
        if (!quiet_output) {
            std::cout << "Game constants: all default\n";
        }
    }

    std::vector<std::string> unlabeledArgsVector = otherArgs.getValue();
    std::list<std::string> unlabeledArgs;
    for (auto a = unlabeledArgsVector.begin(); a != unlabeledArgsVector.end();
         a++) {
        unlabeledArgs.push_back(*a);
    }

    if (mapWidth == 0 && mapHeight == 0) {
        std::vector<unsigned short> mapSizeChoices =
            { 100, 125, 128, 150, 175, 200, 225, 250, 256 };
        std::mt19937 prg(seed);
        std::uniform_int_distribution<unsigned short> size_dist(0, mapSizeChoices.size() - 1);
        auto random_choice = size_dist(prg);
        mapWidth = mapSizeChoices[random_choice];
        mapHeight = mapWidth;
    }

    if (override_names) {
        if (unlabeledArgs.size() < 4 || unlabeledArgs.size() % 2 != 0) {
            std::cout
                << "Invalid number of player parameters with override switch enabled.  Override intended for server use only."
                << std::endl;
            exit(1);
        } else {
            try {
                names = new std::vector<std::string>();
                while (!unlabeledArgs.empty()) {
                    networking.launch_bot(unlabeledArgs.front());
                    unlabeledArgs.pop_front();
                    names->push_back(unlabeledArgs.front());
                    unlabeledArgs.pop_front();
                }
            }
            catch (...) {
                std::cout
                    << "Invalid player parameters with override switch enabled.  Override intended for server use only."
                    << std::endl;
                delete names;
                names = NULL;
                exit(1);
            }
        }
    } else {
        if (unlabeledArgs.size() < 1) {
            std::cout
                << "Please provide the launch command string for at least one bot."
                << std::endl
                << "Use the --help flag for usage details.\n";
            exit(1);
        }
        try {
            while (!unlabeledArgs.empty()) {
                networking.launch_bot(unlabeledArgs.front());
                unlabeledArgs.pop_front();
            }
        }
        catch (...) {
            std::cout
                << "One or more of your bot launch command strings failed.  Please check for correctness and try again."
                << std::endl;
            exit(1);
        }
    }

    if (networking.player_count() > 1 && n_players_for_map_creation != 1) {
        std::cout << std::endl
                  << "Only single-player mode enables specified n-player maps.  When entering multiple bots, please do not try to specify n."
                  << std::endl << std::endl;
        exit(1);
    }

    if (networking.player_count() > 1)
        n_players_for_map_creation = networking.player_count();

    if (n_players_for_map_creation > 6 || n_players_for_map_creation < 1) {
        std::cout << std::endl
                  << "A map can only accommodate between 1 and 6 players."
                  << std::endl << std::endl;
        exit(1);
    }



    //Create game. Null parameters will be ignored.
    my_game = new Halite(mapWidth,
                         mapHeight,
                         seed,
                         n_players_for_map_creation,
                         networking,
                         ignore_timeout);

    std::string outputFilename = replayDirectoryArg.getValue();
#ifdef _WIN32
    if(outputFilename.back() != '\\') outputFilename.push_back('\\');
#else
    if (outputFilename.back() != '/') outputFilename.push_back('/');
#endif
    GameStatistics stats = my_game->run_game(names,
                                             seed,
                                             id,
                                             !noReplaySwitch.getValue(),
                                             outputFilename);
    if (names != NULL) delete names;

    std::string victoryOut;
    if (quiet_output) {
        std::cout << stats;
    } else {
        for (unsigned int player_id = 0;
             player_id < stats.player_statistics.size(); player_id++) {
            auto& player_stats = stats.player_statistics[player_id];
            std::cout
                << "Player #" << player_stats.tag << ", "
                << my_game->get_name(player_stats.tag)
                << ", came in rank #" << player_stats.rank
                << " and was last alive on frame #"
                << player_stats.last_frame_alive
                << ", producing " << player_stats.total_ship_count << " ships"
                << " and dealing " << player_stats.damage_dealt << " damage"
                << "!\n";
        }
    }

    delete my_game;

    return 0;
}
