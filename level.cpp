#include "stdafx.h"

#include "level.h"
#include "location.h"
#include "model.h"

template <class Archive> 
void Level::serialize(Archive& ar, const unsigned int version) { 
  ar& SVAR(squares)
    & SVAR(landingSquares)
    & SVAR(locations)
    & SVAR(tickingSquares)
    & SVAR(creatures)
    & SVAR(model)
    & SVAR(fieldOfView)
    & SVAR(entryMessage)
    & SVAR(name)
    & SVAR(player)
    & SVAR(backgroundLevel)
    & SVAR(backgroundOffset); 
  CHECK_SERIAL;
}  

SERIALIZABLE(Level);

Level::Level(Table<PSquare> s, Model* m, vector<Location*> l, const string& message, const string& n) 
    : squares(std::move(s)), locations(l), model(m), fieldOfView(squares), entryMessage(message), 
    name(n) {
  for (Vec2 pos : squares.getBounds()) {
    squares[pos]->setLevel(this);
    Optional<pair<StairDirection, StairKey>> link = squares[pos]->getLandingLink();
    if (link)
      landingSquares[*link].push_back(pos);
  }
  for (Location *l : locations)
    l->setLevel(this);
}

void Level::addCreature(Vec2 position, PCreature c) {
  Creature* ref = c.get();
  model->addCreature(std::move(c));
  putCreature(position, ref);
}

void Level::putCreature(Vec2 position, Creature* c) {
  creatures.push_back(c);
  CHECK(getSquare(position)->getCreature() == nullptr);
  c->setLevel(this);
  c->setPosition(position);
  //getSquare(position)->putCreatureSilently(c);
  getSquare(position)->putCreature(c);
  notifyLocations(c);
}
  
void Level::notifyLocations(Creature* c) {
  for (Location* l : locations)
    if (c->getPosition().inRectangle(l->getBounds()))
      l->onCreature(c);
}

void Level::replaceSquare(Vec2 pos, PSquare square) {
  if (contains(tickingSquares, getSquare(pos)))
    removeElement(tickingSquares, getSquare(pos));
  Creature* c = squares[pos]->getCreature();
  for (Item* it : squares[pos]->getItems())
    square->dropItem(squares[pos]->removeItem(it));
  squares[pos]->onConstructNewSquare(square.get());
  square->setCovered(squares[pos]->isCovered());
  square->setBackground(squares[pos].get());
  squares[pos] = std::move(square);
  squares[pos]->setPosition(pos);
  squares[pos]->setLevel(this);
  if (c) {
    squares[pos]->putCreatureSilently(c);
  }
  updateVisibility(pos);
}

const Creature* Level::getPlayer() const {
  return player;
}

const Location* Level::getLocation(Vec2 pos) const {
  for (Location* l : locations)
    if (pos.inRectangle(l->getBounds()))
      return l;
  return nullptr;
}

const vector<Location*> Level::getAllLocations() const {
  return locations;
}

vector<Vec2> Level::getLandingSquares(StairDirection dir, StairKey key) const {
  if (landingSquares.count({dir, key}))
    return landingSquares.at({dir, key});
  else
    return {};
}

Vec2 Level::landCreature(StairDirection direction, StairKey key, Creature* creature) {
  vector<Vec2> landing = landingSquares.at({direction, key});
  return landCreature(landing, creature);
}

Vec2 Level::landCreature(StairDirection direction, StairKey key, PCreature creature) {
  Vec2 pos = landCreature(direction, key, creature.get());
  model->addCreature(std::move(creature));
  return pos;
}

Vec2 Level::landCreature(vector<Vec2> landing, PCreature creature) {
  Vec2 pos = landCreature(landing, creature.get());
  model->addCreature(std::move(creature));
  return pos;
}

Vec2 Level::landCreature(vector<Vec2> landing, Creature* creature) {
  CHECK(creature);
  if (entryMessage != "") {
    creature->privateMessage(entryMessage);
    entryMessage = "";
  }
  queue<pair<Vec2, Vec2>> q;
  for (Vec2 pos : randomPermutation(landing))
    q.push(make_pair(pos, pos));
  while (!q.empty()) {
    pair<Vec2, Vec2> v = q.front();
    q.pop();
    if (squares[v.first]->canEnter(creature)) {
      putCreature(v.first, creature);
      return v.second;
    } else
      for (Vec2 next : v.first.neighbors8(true))
        if (next.inRectangle(squares.getBounds()) && squares[next]->canEnterEmpty(creature))
          q.push(make_pair(next, v.second));
  }
  FAIL << "Failed to find any square to put creature";
  return Vec2(0, 0);
}

void Level::throwItem(PItem item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction) {
  vector<PItem> v;
  v.push_back(std::move(item));
  throwItem(std::move(v), attack, maxDist, position, direction);
}

void Level::throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction) {
  CHECK(!item.empty());
  int cnt = 1;
  vector<Vec2> trajectory;
  for (Vec2 v = position + direction;; v += direction) {
    trajectory.push_back(v);
    if (getSquare(v)->itemBounces(item[0].get())) {
        item[0]->onHitSquareMessage(v, getSquare(v), item.size() > 1);
        trajectory.pop_back();
        EventListener::addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
        if (!item[0]->isDiscarded())
          getSquare(v - direction)->dropItems(std::move(item));
        return;
    }
    if (++cnt > maxDist || getSquare(v)->itemLands(extractRefs(item), attack)) {
      EventListener::addThrowEvent(this, attack.getAttacker(), item[0].get(), trajectory);
      getSquare(v)->onItemLands(std::move(item), attack, maxDist - cnt - 1, direction);
      return;
    }
  }
}

void Level::killCreature(Creature* creature) {
  removeElement(creatures, creature);
  getSquare(creature->getPosition())->removeCreature();
  model->removeCreature(creature);
  if (creature->isPlayer())
    setPlayer(nullptr);
}

void Level::updateVisibility(Vec2 changedSquare) {
  fieldOfView.squareChanged(changedSquare);
}

void Level::globalMessage(Vec2 position, const string& ifPlayerCanSee, const string& cannot) const {
  if (player) {
    if (playerCanSee(position))
      player->privateMessage(ifPlayerCanSee);
    else if (player->getPosition().dist8(position) < 30)
      player->privateMessage(cannot);
  }
}

void Level::changeLevel(StairDirection dir, StairKey key, Creature* c) {
  Vec2 fromPosition = c->getPosition();
  removeElement(creatures, c);
  getSquare(c->getPosition())->removeCreature();
  Vec2 toPosition = model->changeLevel(dir, key, c);
  EventListener::addChangeLevelEvent(c, this, fromPosition, c->getLevel(), toPosition);
}

void Level::changeLevel(Level* destination, Vec2 landing, Creature* c) {
  Vec2 fromPosition = c->getPosition();
  removeElement(creatures, c);
  getSquare(c->getPosition())->removeCreature();
  model->changeLevel(destination, landing, c);
  EventListener::addChangeLevelEvent(c, this, fromPosition, destination, landing);
}

void Level::setPlayer(Creature* player) {
  this->player = player;
}

const vector<Creature*>& Level::getAllCreatures() const {
  return creatures;
}

vector<Creature*>& Level::getAllCreatures() {
  return creatures;
}

bool Level::canSee(Vec2 from, Vec2 to) const {
  return fieldOfView.canSee(from, to);
}

bool Level::playerCanSee(Vec2 pos) const {
  return player != nullptr && player->canSee(pos);
}

bool Level::playerCanSee(const Creature* c) const {
  return player != nullptr && player->canSee(c);
}

bool Level::canMoveCreature(const Creature* creature, Vec2 direction) const {
  Vec2 position = creature->getPosition();
  Vec2 destination = position + direction;
  if (!inBounds(destination))
    return false;
  return getSquare(destination)->canEnter(creature);
}

void Level::moveCreature(Creature* creature, Vec2 direction) {
  CHECK(canMoveCreature(creature, direction));
  Vec2 position = creature->getPosition();
  Square* nextSquare = getSquare(position + direction);
  Square* thisSquare = getSquare(position);
  thisSquare->removeCreature();
  creature->setPosition(position + direction);
  nextSquare->putCreature(creature);
  notifyLocations(creature);
}

void Level::swapCreatures(Creature* c1, Creature* c2) {
  Vec2 position1 = c1->getPosition();
  Vec2 position2 = c2->getPosition();
  Square* square1 = getSquare(position1);
  Square* square2 = getSquare(position2);
  square1->removeCreature();
  square2->removeCreature();
  c1->setPosition(position2);
  c2->setPosition(position1);
  square1->putCreature(c2);
  square2->putCreature(c1);
  notifyLocations(c1);
  notifyLocations(c2);
}


vector<Vec2> Level::getVisibleTiles(const Creature* c) const {
  static vector<Vec2> emptyVec;
  if (!c->isBlind())
    return fieldOfView.getVisibleTiles(c->getPosition());
  else
    return emptyVec;
}

unordered_map<Vec2, const ViewObject*> objectList;

void Level::setBackgroundLevel(const Level* l, Vec2 offs) {
  backgroundLevel = l;
  backgroundOffset = offs;
}

static unordered_map<Vec2, const ViewObject*> background;


const Square* Level::getSquare(Vec2 pos) const {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

Square* Level::getSquare(Vec2 pos) {
  CHECK(inBounds(pos));
  return squares[pos].get();
}

void Level::addTickingSquare(Vec2 pos) {
  Square* s = squares[pos].get();
  if (!contains(tickingSquares, s))
    tickingSquares.push_back(s);
}
  
vector<Square*> Level::getTickingSquares() const {
  return tickingSquares;
}

Level::Builder::Builder(int width, int height, const string& n) : squares(width, height), heightMap(width, height, 0),
    fog(width, height, 0), attrib(width, height), type(width, height, SquareType(0)), name(n) {
}

bool Level::Builder::hasAttrib(Vec2 posT, SquareAttrib attr) {
  Vec2 pos = transform(posT);
  CHECK(squares[pos] != nullptr);
  return attrib[pos].count(attr);
}

void Level::Builder::addAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].insert(attr);
}

void Level::Builder::removeAttrib(Vec2 pos, SquareAttrib attr) {
  attrib[transform(pos)].erase(attr);
}

Square* Level::Builder::getSquare(Vec2 pos) {
  return squares[transform(pos)].get();
}
    
SquareType Level::Builder::getType(Vec2 pos) {
  return type[transform(pos)];
}

void Level::Builder::putSquare(Vec2 pos, SquareType t, Optional<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void Level::Builder::putSquare(Vec2 pos, SquareType t, vector<SquareAttrib> at) {
  putSquare(pos, SquareFactory::get(t), t, at);
}

void Level::Builder::putSquare(Vec2 pos, Square* square, SquareType t, Optional<SquareAttrib> attr) {
  putSquare(pos, square, t, attr ? vector<SquareAttrib>({*attr}) : vector<SquareAttrib>());
}

void Level::Builder::putSquare(Vec2 posT, Square* square, SquareType t, vector<SquareAttrib> attr) {
  Vec2 pos = transform(posT);
  CHECK(!contains({SquareType::UP_STAIRS, SquareType::DOWN_STAIRS}, type[pos])) << "Attempted to overwrite stairs";
  square->setPosition(pos);
  if (squares[pos])
    square->setBackground(squares[pos].get());
  squares[pos].reset(std::move(square));
  for (SquareAttrib at : attr)
    attrib[pos].insert(at);
  type[pos] = t;
}

void Level::Builder::addLocation(Location* l, Rectangle area) {
  l->setBounds(area.apply([this](Vec2 v) { return transform(v); }));
  locations.push_back(l);
}

void Level::Builder::setHeightMap(Vec2 pos, double h) {
  heightMap[transform(pos)] = h;
}

void Level::Builder::setFog(Vec2 pos, double value) {
  fog[transform(pos)] = value;
}

double Level::Builder::getHeightMap(Vec2 pos) {
  return heightMap[transform(pos)];
}

void Level::Builder::setCovered(Vec2 pos) {
  covered.insert(transform(pos));
}

void Level::Builder::putCreature(Vec2 pos, PCreature creature) {
  creature->setPosition(transform(pos));
  creatures.push_back(NOTNULL(std::move(creature)));
}

bool Level::Builder::canPutCreature(Vec2 posT, Creature* c) {
  Vec2 pos = transform(posT);
  if (!squares[pos]->canEnter(c))
    return false;
  for (PCreature& c : creatures) {
    if (c->getPosition() == pos)
      return false;
  }
  return true;
}

void Level::Builder::setMessage(const string& message) {
  entryMessage = message;
}

PLevel Level::Builder::build(Model* m, LevelMaker* maker, bool surface) {
  CHECK(mapStack.empty());
  maker->make(this, squares.getBounds());
  for (Vec2 v : heightMap.getBounds()) {
    squares[v]->setHeight(heightMap[v]);
    if (covered.count(v) || !surface) {
      Debug() << "Covered " << v;
      squares[v]->setCovered(true);
    } else
      squares[v]->setFog(fog[v]);
  }
  PLevel l(new Level(std::move(squares), m, locations, entryMessage, name));
  for (PCreature& c : creatures) {
    Vec2 pos = c->getPosition();
    l->addCreature(pos, std::move(c));
  }
  return l;
}

Vec2::LinearMap Level::Builder::identity() {
  return [](Vec2 v) { return v; };
}

Vec2::LinearMap Level::Builder::deg90(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.getTopLeft();
    return bounds.getTopLeft() + Vec2(v.y, v.x);
  };
}

Vec2::LinearMap Level::Builder::deg180(Rectangle bounds) {
  return [bounds](Vec2 v) {
    return bounds.getTopLeft() - v + bounds.getBottomRight() - Vec2(1, 1);
  };
}

Vec2::LinearMap Level::Builder::deg270(Rectangle bounds) {
  return [bounds](Vec2 v) {
    v -= bounds.getTopRight() - Vec2(1, 0);
    return bounds.getTopLeft() + Vec2(v.y, -v.x);
  };
}

void Level::Builder::pushMap(Rectangle bounds, Rot rot) {
  switch (rot) {
    case CW0: mapStack.push_back(identity()); break;
    case CW1: mapStack.push_back(deg90(bounds)); break;
    case CW2: mapStack.push_back(deg180(bounds)); break;
    case CW3: mapStack.push_back(deg270(bounds)); break;
  }
}

void Level::Builder::popMap() {
  mapStack.pop_back();
}

Vec2 Level::Builder::transform(Vec2 v) {
  for (auto m : mapStack) {
    v = m(v);
  }
  return v;
}

bool Level::inBounds(Vec2 pos) const {
  return pos.inRectangle(getBounds());
}

Rectangle Level::getBounds() const {
  return Rectangle(0, 0, getWidth(), getHeight());
}

int Level::getWidth() const {
  return squares.getWidth();
}

int Level::getHeight() const {
  return squares.getHeight();
}

const string& Level::getName() const {
  return name;
}
