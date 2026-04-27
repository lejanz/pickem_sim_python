# https://github.com/ValveSoftware/counter-strike_rules_and_regs/blob/main/major-supplemental-rulebook.md#Difficulty-score

from typing import List, Tuple, Dict
from copy import deepcopy

NUM_TEAMS = 16
WINS_TO_ADVANCE = 3
LOSSES_TO_ADVANCE = 3

class Team:
    name: str = ""
    pre_stage_seed: int = 0
    buchholtz: int = 0
    wins: int = 0
    losses: int = 0
    opponents: List[str] = []

    def __init__(self):
        self.name = input("name:")
        self.pre_stage_seed = input("seed:")
        self.opponents = []

    def __init__(self, name, seed):
        self.name = name
        self.pre_stage_seed = seed
        self.opponents = []

    def record(self):
        return f'{self.wins}-{self.losses}'
    
    def advanced(self):
        return self.wins > 2
    
    def knocked_out(self):
        return self.losses > 2

class Swiss:
    teams: List[Team] = [] # sorted by seed (index 0 is seed 1)
    round: int = 1
    probability: float = 1.0
    matchups = List[Tuple[Team, Team]]

    def __init__(self, teams: List[Tuple[Team, Team]]):
        self.teams = teams
        self.update_matchups()

    def get_team_by_name(self, name: str):
        for team in self.teams:
            if team.name == name:
                return team

        raise KeyError

    def update_buchholtz(self):
        for team in self.teams:
            team.buchholtz = 0
            for opponent_name in team.opponents:
                opponent = self.get_team_by_name(opponent_name)
                team.buchholtz += opponent.wins - opponent.losses

    def reseed(self):
        self.update_buchholtz()

        def sort_key(team: Team):
            return (-team.wins,           # wins descending
                    team.losses,          # losses ascending
                    -team.buchholtz,      # buchholtz descending
                    team.pre_stage_seed)  # initial seeding ascending

        self.teams.sort(key=sort_key)

    def get_record_sorted_teams(self, matchup_only=False) -> Dict[str, List[Team]]:
        record_sorted_teams = {}
        for team in self.teams:
            if matchup_only and team.advanced() or team.knocked_out():
                continue
            if team.record() not in record_sorted_teams:
                record_sorted_teams[team.record()] = [team]
            else:
                record_sorted_teams[team.record()].append(team)
        return record_sorted_teams
    
    def update_matchups(self):
        assert 0 < self.round < 6, 'Invalid round!'
        self.matchups = []

        # Round 1 uses fixed initial matchups - 1v9, 2v10 ... 8v16
        if self.round == 1:
            self.matchups = [(self.teams[i], self.teams[i+8]) for i in range(8)]
            return

        # Rounds 2 and 3 - the highest team plays the lowest available team that does not result in a rematch
        if self.round in [2, 3]:
            record_sorted_teams = self.get_record_sorted_teams(matchup_only=True)
            for available_teams in record_sorted_teams.values():
                while True:
                    if len(available_teams) < 2:
                        break
                    
                    # grab the higest seeded team
                    team = available_teams.pop(0)

                    # find the lowest seeded opponent with the same record this team has not played
                    for opponent_idx, potential_opponent in reversed(list(enumerate(available_teams))):
                        if (
                            potential_opponent.record() == team.record() and 
                            potential_opponent.name not in team.opponents
                            ):
                            available_teams.pop(opponent_idx)
                            self.matchups.append((team, potential_opponent))
                            break

                        # This code should be unreachable with opponent_idx = 0
                        assert not opponent_idx == 0, 'No valid opponent found!'

        # Rounds 4 and 5 use valve defined priority list   
        else:
            matchup_options = [
                [(1, 6), (2, 5), (3, 4)],
                [(1, 6), (2, 4), (3, 5)],
                [(1, 5), (2, 6), (3, 4)],
                [(1, 5), (2, 4), (3, 6)],
                [(1, 4), (2, 6), (3, 5)],
                [(1, 4), (2, 5), (3, 6)],
                [(1, 6), (2, 3), (4, 5)],
                [(1, 5), (2, 3), (4, 6)],
                [(1, 3), (2, 6), (4, 5)],
                [(1, 3), (2, 5), (4, 6)],
                [(1, 4), (2, 3), (5, 6)],
                [(1, 3), (2, 4), (5, 6)],
                [(1, 2), (3, 6), (4, 5)],
                [(1, 2), (3, 5), (4, 6)],
                [(1, 2), (3, 4), (5, 6)],
            ]
            record_sorted_teams = self.get_record_sorted_teams(matchup_only=True)
            for available_teams in record_sorted_teams.values():
                for row in matchup_options:
                    matchups = []
                    for team0_seed, team1_seed in row:
                        matchup = (available_teams[team0_seed-1], available_teams[team1_seed-1])
                        # check if this is a rematch
                        if matchup[0].name not in matchup[1].opponents:
                            matchups.append(matchup)

                    # if there are no rematches, there will be 3 matchups in the list
                    if len(matchups) == 3:
                        self.matchups.extend(matchups)
                        break
                
                assert len(matchups) == 3, 'No row found which does not result in a rematch!'
            
    def get_matchup(self) -> Tuple[Team, Team]:
        if len(self.matchups) < 1:
            return None
        matchup = self.matchups.pop()
        return (matchup[0].name, matchup[1].name)
    
    def add_result(self, winner_name: str, loser_name: str, probability):
        self.probability *= probability
        winner = self.get_team_by_name(winner_name)
        loser = self.get_team_by_name(loser_name)
        winner.opponents.append(loser_name)
        loser.opponents.append(winner_name)
        winner.wins += 1
        loser.losses += 1

    def round_done(self):
        return len(self.matchups) == 0
        
    def last_round(self):
        return self.round == 2
        #return all([team.advanced() or team.knocked_out() for team in self.teams])

    def advance_round(self):
        # check that all teams have played the correct number of games
        for team in self.teams:
            assert (
                team.wins + team.losses == self.round or
                team.advanced() or team.knocked_out()
            ), f'{team.name} has an invalid match record ({team.record()}, round {self.round})'
        
        self.round += 1
        self.reseed()
        #self.print_standings()
        self.update_matchups()

    # return dictionary of standings
    def get_standings(self) -> Dict[str, str]:
        return {team.name: team.record() for team in self.teams}

    def print_standings(self):
        print('Standings:')
        for team in self.teams:
            print(f'{team.name}: {team.record()}, {team.buchholtz}')
        return

teams = [
    "furia",
    "vitality",
    "falcons",
    "mongolz",
    "mouz",
    "spirit",
    "g2", 
    "pain",
    "navi",
    "faze",
    "b8",
    "imperial",
    "parivision",
    "liquid",
    "passion",
    "3dmax",
]

bracket = Swiss([Team(name, seed) for seed, name in enumerate(teams)])

team_results = {team: {} for team in teams}

def iteration(bracket_in: Swiss, team_results: Dict[str, Dict[str, int]]):
    if bracket_in.round_done():
        if not bracket_in.last_round():
            bracket_in.advance_round()
        else:
            standings = bracket_in.get_standings()
            for team, result in standings.items():
                if result in team_results[team]:
                    team_results[team][result] += 1
                else:
                    team_results[team][result] = 1
            return

    team0, team1 = bracket_in.get_matchup()
    p_win = 0.5 # TODO replace with LUT

    bracket0 = deepcopy(bracket_in)
    bracket0.add_result(winner_name=team0, loser_name=team1, probability=p_win)
    iteration(bracket0, team_results)

    bracket1 = deepcopy(bracket_in)
    bracket1.add_result(winner_name=team1, loser_name=team0, probability=1-p_win)
    iteration(bracket1, team_results)

total_results = iteration(bracket, team_results)
print(team_results)



# for round in range(5):
#     print(f'Round', bracket.round)
#     for team0, team1 in bracket.matchups:
#         team0.opponents.append(team1.name)
#         team1.opponents.append(team0.name)
#         winner = input(f"Choose winner: 0:{team0.name} vs 1:{team1.name} ")
#         if winner == '0':
#             team0.wins += 1
#             team1.losses += 1
#         else:
#             team1.wins += 1
#             team0.losses += 1
    
#     bracket.advance_round()
    
#     print('')


            
                


        

        



