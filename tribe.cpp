#include "stdafx.h"


using namespace std;

class KillEveryone : public Tribe {
  public:
  KillEveryone() : Tribe("", false) {}
  virtual double getStanding(const Creature*) const override {
    return -1;
  }
};

Tribe* const Tribe::monster = new Tribe("", false);
Tribe* const Tribe::pest = new Tribe("", false);
Tribe* const Tribe::wildlife = new Tribe("", false);
Tribe* const Tribe::elven = new Tribe("elves", true);
Tribe* const Tribe::goblin = new Tribe("goblins", true);
Tribe* const Tribe::dwarven = new Tribe("dwarves", true);
Tribe* const Tribe::player = new Tribe("", false);
Tribe* const Tribe::killEveryone = new KillEveryone();

Tribe::Tribe(const string& n, bool d) : diplomatic(d), name(n) {
}

const string& Tribe::getName() {
  return name;
}

double Tribe::getStanding(const Creature* c) const {
  if (c->getTribe() == this)
    return 1;
  if (standing.count(c)) 
    return standing.at(c);
  if (enemyTribes.count(c->getTribe()))
    return -1;
  return 0;
}

void Tribe::initStanding(const Creature* c) {
  standing[c] = getStanding(c);
}

void Tribe::addEnemy(Tribe* t) {
  enemyTribes.insert(t);
  t->enemyTribes.insert(this);
}

void Tribe::makeSlightEnemy(const Creature* c) {
  standing[c] = -0.001;
}

static const double killBonus = 0.1;
static const double killPenalty = 0.5;
static const double attackPenalty = 0.2;
static const double thiefPenalty = 0.5;
static const double importantMemberMult = 0.5 / killBonus;

double Tribe::getMultiplier(const Creature* member) {
  if (contains(importantMembers, member))
    return importantMemberMult;
  else
    return 1;
}


void Tribe::onKillEvent(const Creature* member, const Creature* attacker) {
  if (member->getTribe() != this || attacker == nullptr)
    return;
  initStanding(attacker);
  standing[attacker] -= killPenalty * getMultiplier(member);
  for (Tribe* t : enemyTribes)
    if (t->diplomatic) {
      t->initStanding(attacker);
      t->standing[attacker] += killBonus * getMultiplier(member);
    }
}

void Tribe::onAttackEvent(const Creature* member, const Creature* attacker) {
  if (member->getTribe() != this)
    return;
  if (contains(attacks, make_pair(member, attacker)))
    return;
  attacks.emplace_back(member, attacker);
  initStanding(attacker);
  standing[attacker] -= attackPenalty * getMultiplier(member);
}

void Tribe::addImportantMember(const Creature* c) {
  importantMembers.push_back(c);
}

vector<const Creature*> Tribe::getImportantMembers() {
  return importantMembers;
}

void Tribe::onItemsStolen(const Creature* attacker) {
  if (diplomatic) {
    initStanding(attacker);
    standing[attacker] -= thiefPenalty;
  }
}

void Tribe::init() {
  EventListener::addListener(goblin);
  EventListener::addListener(dwarven);
  EventListener::addListener(elven);
  elven->addEnemy(goblin);
  elven->addEnemy(dwarven);
  goblin->addEnemy(dwarven);
  goblin->addEnemy(wildlife);
  player->addEnemy(monster);
  player->addEnemy(goblin);
  player->addEnemy(pest);
  monster->addEnemy(wildlife);
  wildlife->addEnemy(player);
  wildlife->addEnemy(goblin);
  wildlife->addEnemy(monster);
}
