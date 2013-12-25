#ifndef _VIEW_H
#define _VIEW_H

#include <string>
#include <vector>
#include <map>

#include "util.h"
#include "action.h"
#include "collective_action.h"
#include "debug.h"
#include "view_object.h"

class CreatureView;
class Level;

class View {
  public:

  /** Does all the library specific init.*/
  virtual void initialize() = 0;

  /** Displays a splash screen in an active loop until \paramname{ready} is set to true in another thread.*/
  virtual void displaySplash(bool& ready) = 0;

  /** Shutdown routine.*/
  virtual void close() = 0;

  /** Reads the game state from \paramname{creatureView} and refreshes the display.*/
  virtual void refreshView(const CreatureView* creatureView) = 0;

  /** Scrolls back to the center of the view on next refresh.*/
  virtual void resetCenter() = 0;

  //@{
  /** Adds a new message to the box. The message is remembered between refreshes,
      and is discarded when appropriate.*/
  virtual void addMessage(const string& message) = 0;
  virtual void addImportantMessage(const string& message) = 0;
  //@}

  /** Clears the message box.*/
  virtual void clearMessages() = 0;

  /** Reads the keyboard in a blocking manner. Used for turn-based game play.*/
  virtual Action getAction() = 0;

  /** Reads input in a non-blocking manner.*/
  virtual CollectiveAction getClick() = 0;

  /** Returns whether a travel interrupt key is pressed at a given moment.*/
  virtual bool travelInterrupt() = 0;

  /** Draws a window with some options for the player to choose. \paramname{index} indicates the highlighted item. 
      Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<int> chooseFromList(const string& title, const vector<string>& options, int index = 0) = 0;

  /** Let's the player choose a direction from the main 8. Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<Vec2> chooseDirection(const string& message) = 0;

  /** Asks the player a yer-or-no question.*/
  virtual bool yesOrNoPrompt(const string& message) = 0;

  /** Draws a window with some text. The text is formatted to fit the window.*/
  virtual void presentText(const string& title, const string& text) = 0;

  /** Draws a window with a list of items.*/
  virtual void presentList(const string& title, const vector<string>& options, bool scrollDown = false) = 0;

  /** Let's the player choose a number. Returns Nothing() if the player cancelled the choice.*/
  virtual Optional<int> getNumber(const string& title, int max) = 0;

  /** Draws an animation of an object between two locations on a map.*/
  virtual void animateObject(vector<Vec2> trajectory, ViewObject object) = 0;

  /** Returns the current real time in milliseconds. The clock is stopped on blocking keyboard input,
      so it can be used to sync game time in real-time mode.*/
  virtual int getTimeMilli() = 0;

  /** Stops the real time clock.*/
  virtual void stopClock() = 0;

  /** Sets the real time clock.*/
  virtual void setTimeMilli(int) = 0;

  /** Continues the real time clock after it had been stopped.*/
  virtual void continueClock() = 0;

  /** Returns whether the real time clock is currently stopped.*/
  virtual bool isClockStopped() = 0;

  /** Returns a default View that additionally logs all player actions into a file.*/
  static View* createLoggingView(ofstream& of);

  /** Returns a default View that reads all player actions from a file instead of the keyboard.*/
  static View* createReplayView(ifstream& ifs);

  /** Marks a given menu item to be displayed as a title.*/
  static string getTitlePrefix(const string& name);

  /** Returns whether a menu item is marked as a title.*/
  static bool hasTitlePrefix(const string& name);

  /** Removes the title prefix from a menu item.*/
  static string removeTitlePrefix(const string& name);

  /** Represents all the game information displayed around the map window.*/
  class GameInfo {
    public:
    enum class InfoType { PLAYER, BAND} infoType;

    class BandInfo {
      public:
      int number;
      string name;
      string warning;
      vector<tuple<string, ViewObject, bool>> buttons;
      string monsterHeader;
      vector<const Creature*> creatures;
      vector<const Creature*> enemies;
      map<const Creature*, string> tasks;
      int numGold = 0;
      int activeButton = 0;
      double time;
      bool gatheringTeam = false;
      vector<const Creature*> team;
    } bandInfo;

    class PlayerInfo {
      public:
      int speed;
      int defense;
      int attack;
      int strength;
      int dexterity;
      bool bleeding;
      double time;
      int numGold;
      string playerName;
      vector<string> adjectives;
      string title;
      string levelName;
      string weaponName;
      double elfStanding;
      double dwarfStanding;
      double goblinStanding;
    } playerInfo;
  } gameInfo;
};

#endif