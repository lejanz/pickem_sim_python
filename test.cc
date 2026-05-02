#include <iomanip>
#include <assert.h>
#include <random>

#include "pickem_sim.h"


int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_wl(0, 3);
    std::uniform_int_distribution<> rand_team(0, 16);
    // Check win loss packing function
    // assert(pack_win_loss(0, 0) == 0);
    // assert(pack_win_loss(2, 2) == 0xA);
    // assert(pack_win_loss(3, 1) == 0xD);
    // assert(pack_win_loss(1, 2) == 0x6);

    // for (uint8_t team_id = 0; team_id < 16; team_id++)
    // {
    //     uint64_t team = TEAM_FLAG(team_id);
    //     std::cout << std::hex << (team) << std::endl;
    // }

    Swiss bracket;
    uint8_t matchups[9] = {};
    bracket.get_matchups(matchups);
    print_matchups(matchups);

    for (uint8_t i = 0; i < 8; i++)
    {
        bracket.play_match(TEAM1(matchups[i]), TEAM0(matchups[i]), 1.0f);
    }
    
    bracket.print_standings();
    
    bracket.round++;
    bracket.get_matchups(matchups);
    print_matchups(matchups);

    for (uint8_t i = 0; i < 8; i++)
    {
        bracket.play_match(TEAM1(matchups[i]), TEAM0(matchups[i]), 1.0f);
    }
    
    bracket.print_standings();
    
    bracket.round++;
    bracket.get_matchups(matchups);
    print_matchups(matchups);

    // for (uint64_t team_id = 0; team_id < 16; team_id++)
    // {
    //     int wins = rand_wl(gen);
    //     int losses = rand_wl(gen);
    //     //std::cout << team_id << " WL " << wins << losses << std::endl;
    //     bracket.set_team_record(team_id, wins, losses);
    //     bracket.opponents[team_id] = TEAM_FLAG(rand_team(gen));
    // }

    // //uint64_t i = 0;
    // for (i = 0; i < 10000000; i++)
    // {
    //     bracket.team_wl = 0;
    //     int buchholtz = 0;
    //     for (uint8_t team_id = 0; team_id < 5; team_id++)
    //     {
    //         int wins = rand_wl(gen);
    //         int losses = rand_wl(gen);
    //         //std::cout << wins << losses << std::endl;
    //         buchholtz += (wins - losses);
    //         bracket.set_team_record(team_id, wins, losses);
    //     }
    //     int64_t fast_buchholtz = bracket.fast_buchholtz(MAGIC_JOHNSON);
    //     assert(bracket.fast_buchholtz(MAGIC_JOHNSON) == buchholtz);
    // }
    // std::cout << "Done!" << i << std::endl;




    // std::cout << "Team WL: " << std::hex << bracket.team_wl << std::dec << std::endl;
    // //std::cout << "Team losses: " << std::hex << bracket.team_losses << std::dec << std::endl;
    // // uint64_t difficulty_score = bracket.get_difficulty_score();
    // // std::cout << "Difficulty score: " << std::hex << difficulty_score << std::dec << std::endl;

    // std::cout << "Buchholtz: " << buchholtz << std::endl;

    // int fast_buchholtz = bracket.fast_buchholtz(MAGIC_JOHNSON);
    // std::cout << "Fast Buchholtz: " << fast_buchholtz << std::endl;

    //uint64_t team = TEAM_FLAG(5);
    //int64_t difficulty_score = -1;
    // uint64_t win_loss =  PACK_WL(1, 3) * INV_TEAM_FLAG(0) | INV_TEAM_FLAG(2) * PACK_WL(1, 3) | INV_TEAM_FLAG(1) * PACK_WL(3, 1);
    // uint64_t difficulty_score = ((win_loss >> 2) & (MAGIC_JOHNSON * 0x3)) + ((~(win_loss & MASK_ALL(0x3)) & MASK_ALL(0x7)) + MAGIC_JOHNSON & MASK_ALL(0x7)) & MASK_ALL(0x7);
    // std::cout << std::hex << win_loss << std::endl;
    // std::cout << std::hex << difficulty_score << std::endl;

    // uint64_t opponents = TEAM_FLAG(0) | TEAM_FLAG(1);
    // uint64_t buchholtz = (opponents * difficulty_score) >> 60 & 0x7;
    // std::cout << std::hex << buchholtz << std::endl;

    return 0;
}