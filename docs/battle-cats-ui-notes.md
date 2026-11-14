# The Battle Cats: UI notes

Notes from browsing the user's own copy of The Battle Cats on their phone (2026-11-14): what each menu looks like, how
it moves between screens, and what makes it feel good to navigate. They are references for Fish Fighters' menus, not
things to copy one to one: Fish Fighters keeps its own underwater look (steel, coral, nacre) and its own names.

## What makes it feel good

1. **One frame that never leaves.** Every menu has the same top bar (the screen's name on the left, XP on the right)
   and the same bottom bar (the back button sitting on its left end, the shop and Cat Food on the right). Changing
   screens never removes the frame: only the content between the bars changes, and the bars swap their own texts and
   buttons in place (Cat Base's "Store" becomes Storage's "Use all / Exchange all").
2. **Every screen change is a small scene, not a cut.** About 0.6 to 1 s, always the same metaphor: the base's two
   green doors slide apart to reveal the next screen, after the old buttons slid out to the left. Going back plays it
   the other way round.
3. **A press is answered at once.** The pressed button flashes brighter and gets a pink frame before anything else
   moves, so the tap is acknowledged even while the transition plays.
4. **The current choice is always marked the same way.** One pink frame that pulses: the selected card, the selected
   tab, the main button of the screen (Attack! on the stage select). Everything not selected is dimmed, darkened or
   see-through.
5. **Screens are full.** Each region has a job: lineup and list, carousel and description, counters, shortcuts. No
   large empty areas; the background pattern fills what's left.
6. **Lists are carousels you can fling.** Swipe anywhere on the screen (not only on the cards): the selected card is
   big in the middle, its neighbours smaller and darker, a row of page dots says where you are in the list.
7. **The hub talks.** The mascot in the corner has a speech bubble with rotating tips (a new one each visit).
8. **Things that want attention say so.** Red count badges on buttons (Rare Capsule "7"), little yellow callout
   bubbles above them ("... Available Now!"), a rotating banner of events at the top right.
9. **Numbers move.** Counters roll to their new value (XP after a battle) instead of jumping.
10. **Chunky, readable type.** Big outlined white titles, yellow digits with a dark outline, every icon with its name
    under it.

## Screen by screen

### Cat Base (the hub)
- Left column: three big yellow buttons (Start!!, Upgrade, Equip), then three icon tiles with their names (Menu,
  GAMATOTO, Missions).
- Bottom middle: a wooden shelf holding round buttons (Storage, the capsules), each with a callout bubble above it.
- Right: the mascot, cut by the screen's corner, with its tip bubble.
- Top: the screen name with a swap icon, a news ticker, the event banner (with a small "i"), XP. A big counter (4197)
  with an "i" at the top left of the content.
- Background: the base's wall, a patterned wallpaper, split by the two doors in the middle.

### Organize Cats (Equip)
- **Top half: the lineup.** Two rows of five slots (Page 1, Page 2), each slot the cat's face with its cost in yellow
  digits at its bottom right. Big yellow arrows on both sides switch between saved lineups ("Slot 2" pill with a "+",
  page dots above). The Cat Cannon chosen at the top right.
- **Bottom half: the carousel.** A "Characters" tab with a sort pill (Basic Order), category cards first (Rare Cats,
  Special Cats), then the cats. The selected card is bigger with a pulsing pink border, the others are dark; a red
  "In use" stamp on cats already in the lineup; a round swap button on the selected card (its forms).
- Right: a big "Unit Info" button and a favourites button.
- Bottom bar: "Upgrade" (jumps to the Upgrade screen) and "CatCombo List".

### Upgrade
- Top: page dots across the whole width (where you are among all the cats).
- Under them: the same carousel as Organize (tab, sort pill, category cards, selected card big and lit, the others
  dark), the catseye counts at the top right.
- Bottom: a black band, then a wide brown rounded panel with the cat's description (with a "more" arrow at its bottom
  right). The level-up button and its XP price come with it for cats not at their max.

### Storage
- Full screen: the cats float around a blue tank, slowly.
- An "All" filter at the top left, a count "29/64" at the bottom right.
- Bottom bar: "Use all" and "Exchange all for NP/XP".

### Menu (popup)
- The whole screen darkens; a light grey panel with its title on a tab ("Menu"), a round white "x" on its top right
  corner, a grid of two columns of buttons (an icon on the left, the name), a "? Help" button at the bottom right.
- It appears at once (no long animation): menus that are only lists open instantly.

### Capsules (gacha)
- A row of tabs at the top (the selected one framed in pink).
- Big yellow arrows on both sides, the capsule machine in the middle, "Try once!" with the tickets left at the bottom
  right, a Storage shortcut at the bottom left; the tickets counter is in the bottom bar.

### Stage Select
- A strip of stage tabs along the top: the selected one bigger, solid white with a heavy border, the others smaller and
  see-through; side arrows. Under the selected tab: its energy cost and how many times it was cleared.
- The map behind it, with the path of red dots; the player's cat stands on the selected stage.
- A swipe anywhere on the map moves to the next or previous stage (the strip, the map and the cat follow).
- Right: Energy, then a huge Attack! with a pulsing pink frame (the main action is pre-selected), an Equip shortcut
  with the lineup's slot number; a row of item buttons with their counts at the bottom.

### Battle start
- Attack! flashes; the menu's bars and buttons slide away so only the map is left.
- The mascot peeks in from the bottom right; a big "Battle" pops in, then "The Battle Begins!!" with the "!!" scaling
  in, then the battle loads.

### Battle HUD
- Top left: a round pause button and the stage name. Top right: the money "2017/6000" and a round speed button under
  it.
- Bottom: the ground's zigzag edge, the deploy grid in the middle (2 rows of 5, the same faces as the lineup with their
  costs), the worker level-up at the bottom left (a round button: its level and price), the cannon at the bottom right
  (grey while it charges).
- The base's health above each base.

### Victory
- The HUD slides away (the deploy rows drop out, the pause and the money go).
- "Victory!" in huge letters over a see-through dark band across the screen, the reward on the band ("Gained ... XP!!"),
  one yellow "Ok" and a small share button. The battle keeps playing behind it (the cats cheer, the base falls apart).

### Back to the base
- A black wipe with a zigzag edge sweeps across, then pulls away to reveal the Cat Base.
- The base's buttons slide in from the left, the mascot rises from the bottom, then its tip bubble appears; the XP
  counter rolls up to the new amount.

## For Fish Fighters

| The Battle Cats | Fish Fighters | |
|---|---|---|
| Top bar on every menu | UITitleBar (steel, violet, nacre) | done |
| Bottom bar (back button, shop, premium currency) | UIBottomBar | being added |
| Lineup on top, list at the bottom (Organize) | Equip | being changed |
| Page dots on the carousels | Upgrade, Equip | being added |
| Swipe anywhere, fling with momentum | Upgrade, Equip, chapter map | done |
| Stage strip: selected tab bigger and solid, others see-through | chapter map | done |
| Doors opening between screens | the bubble wave | ours; could open the hub's porthole instead |
| Pressed button flashes before the transition | buttons | to do |
| One pulsing frame for the current choice (and the main button) | yellow borders, breathing main buttons | partly |
| Mascot with rotating tips on the hub | the Fish's tips | done |
| Badges and callout bubbles on hub buttons | | to do ("New fish!" on Upgrade, "FREE" on the Clam Room) |
| Menu popup: dim, tab title, round x, icon grid, Help | Options, pause | could match |
| "The Battle Begins!!" | UIBattleIntro | compare |
| Victory band over the running battle, one Ok | results panel | to do |
| Zigzag wipe back to the base, buttons slide in | | to do |
| Rolling counters | shells, scales | done |
