#include <iomanip>
#include <assert.h>
#include <random>

#include "pickem_sim.h"

int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_wl(0, 3);
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
    int buchholtz = 0;
    for (uint8_t team_id = 0; team_id < 3; team_id++)
    {
        int wins = rand_wl(gen);
        int losses = rand_wl(gen);
        std::cout << wins << losses << std::endl;
        buchholtz += (wins - losses);
        bracket.set_team_record(team_id, wins, losses);
    }

    std::cout << "Packed WL: " << std::hex << bracket.win_loss << std::endl;

    uint64_t difficulty_score = bracket.get_difficulty_score();
    std::cout << "Difficulty score: " << std::hex << difficulty_score << std::dec << std::endl;

    std::cout << "Buchholtz: " << buchholtz << std::endl;

    int fast_buchholtz = bracket.fast_buchholtz(difficulty_score, MAGIC_JOHNSON);
    std::cout << "Fast Buchholtz: " << fast_buchholtz << std::endl;

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