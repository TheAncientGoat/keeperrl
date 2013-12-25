#include "stdafx.h"


RangedWeapon::RangedWeapon(ViewObject o, const ItemAttributes& attr) : Item(o, attr) {}

void RangedWeapon::fire(Creature* c, Level* l, PItem ammo, Vec2 dir) {
  int toHitVariance = 10;
  int attackVariance = 15;
  int toHit = Random.getRandom(-toHitVariance, toHitVariance) + 
    c->getAttr(AttrType::THROWN_TO_HIT) +
    ammo->getModifier(AttrType::THROWN_TO_HIT) +
    getAccuracy();
  int damage = Random.getRandom(-attackVariance, attackVariance) + 
    c->getAttr(AttrType::THROWN_DAMAGE) +
    ammo->getModifier(AttrType::THROWN_DAMAGE);
  l->throwItem(std::move(ammo), Attack(c, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
        AttackType::SHOOT, toHit, damage),
      20, c->getPosition(), dir);
}
