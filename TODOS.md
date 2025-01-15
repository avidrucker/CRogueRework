# To-Do's

- [ ] make corridor bend points (corners) happen at a random point, rather than always at the beginning or end of the corridor's X or Y path
- [x] always generate 9 rooms, and then randomly delete 0-3 rooms, replacing with corridors
- [ ] when placing corridors to a deleted room, pick a point somewhere randomly inside of where the room would have been to connect the corridors
- [ ] make some doors "secret" that appear as normal walls, requiring the player to search with the 's' key to reveal them
- [ ] implement the 's' command for the player to search for secret doors (1/5 chance of succeeding)
- [ ] fix bug where exit is sometimes placed in the same room as the player (it should always be a different room, ideally far away from the player), probably the issue is that node only rooms are not being traversed as rooms