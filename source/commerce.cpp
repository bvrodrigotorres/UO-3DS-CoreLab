// SPDX-License-Identifier: MIT
#include "commerce.hpp"
#include <cctype>
namespace lab {
namespace {
std::vector<unsigned>catalogue(const Npc&n){
    if(!n.alive||n.animal||n.hostile)return {};
    std::string title=n.title;std::transform(title.begin(),title.end(),title.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto is=[&](const char*s){return title.find(s)!=std::string::npos;};
    if(is("smith")||is("armorer")||is("armourer"))return {5,16,17,18,19,20,21,14,15,26,27};
    if(is("jewel"))return {28,29};
    if(is("tailor")||is("weaver"))return {6,7,8,9,10,11,12,13,23};
    if(is("butcher"))return {22};
    if(is("tanner"))return {19,14,15,23,26};
    if(is("bowyer"))return {24};
    if(is("healer")||is("alchemist"))return {3,4,25};
    if(is("baker")||is("cook")||is("innkeeper")||is("tavern keeper")||is("barkeeper"))return {1,2};
    if(is("provisioner")||is("merchant"))return {1,2,3,4,5,7,8,10,12,28,29};
    return {};
}
std::vector<Skill>teaching(const Npc&n){
    if(!n.alive||n.animal||n.hostile)return {};
    std::string title=n.title;std::transform(title.begin(),title.end(),title.begin(),[](unsigned char c){return char(std::tolower(c));});
    auto is=[&](const char*s){return title.find(s)!=std::string::npos;};
    using S=Skill;
    // Profession associations from UO's official Skill Titles & Order trainer table.
    if(is("healer")||is("bloodletter"))return {S::Healing,S::Anatomy,S::Forensics,S::SpiritSpeak};
    if(is("warrior")||is("fighter")||is("paladin")||is("mercenary")||is("weapons trainer"))return {S::Tactics,S::Swords,S::Macing,S::Fencing,S::Parrying,S::ArmsLore};
    if(is("swordsman"))return {S::Swords,S::Tactics};
    if(is("fencer"))return {S::Fencing,S::Tactics};
    if(is("wrestler"))return {S::Wrestling,S::Tactics};
    if(is("smith")||is("armorer")||is("armourer"))return {S::Blacksmith,S::ArmsLore};
    if(is("alchemist")||is("herbalist"))return {S::Alchemy,S::TasteID};
    if(is("archmage")||is("mage")||is("wizard"))return {S::Magery,S::Meditation,S::EvalInt,S::Inscription,S::MagicResist};
    if(is("tailor")||is("weaver"))return {S::Tailoring};
    if(is("animal trainer"))return {S::AnimalLore,S::AnimalTaming,S::Veterinary};
    if(is("veterinar")||is("the vet"))return {S::AnimalLore,S::Veterinary};
    if(is("fur trader")||is("furtrader"))return {S::AnimalLore,S::AnimalTaming,S::Camping};
    if(is("scribe"))return {S::EvalInt,S::Inscription};
    if(is("artist")||is("sculptor"))return {S::Anatomy};
    if(is("ranger"))return {S::Archery,S::AnimalLore,S::AnimalTaming,S::Tracking,S::Camping,S::Hiding};
    if(is("bowyer"))return {S::Bowcraft,S::Archery};
    if(is("bard")||is("minstrel"))return {S::Musicianship,S::Peacemaking,S::Provocation,S::Discordance};
    if(is("carpenter"))return {S::Carpentry,S::Lumberjacking};
    if(is("baker")||is("cook"))return {S::Cooking,S::TasteID};
    if(is("fisher"))return {S::Fishing};
    if(is("tinker"))return {S::Tinkering};
    if(is("miner"))return {S::Mining};
    if(is("mapmaker")||is("cartographer"))return {S::Cartography};
    if(is("jeweler")||is("jeweller")||is("merchant guild"))return {S::ItemID};
    if(is("shepherd"))return {S::Herding,S::Camping};
    if(is("rancher"))return {S::AnimalLore,S::Herding,S::Veterinary};
    if(is("beggar"))return {S::Begging,S::Snooping,S::Stealing};
    if(is("thief"))return {S::Hiding,S::Snooping,S::Stealing,S::Lockpicking,S::DetectHidden};
    return {};
}

}
bool Commerce::update(const World&w,const Npcs&npcs,const Actor&a){
    int nearest=-1,best=10000;std::vector<unsigned>next;
    for(size_t i=0;i<npcs.all().size();++i){const auto&n=npcs.all()[i];int dx=n.position.x-a.x,dy=n.position.y-a.y;
        if(std::max(std::abs(dx),std::abs(dy))>1||!meleeReach(w,a,n.position))continue;
        auto items=catalogue(n);auto skills=teaching(n);int distance=dx*dx+dy*dy;
        if((!items.empty()||!skills.empty())&&(distance<best||(distance==best&&int(i)==vendor))){best=distance;nearest=int(i);next=std::move(items);}}
    std::string nextIdentity=nearest>=0?npcs.all()[nearest].name+" "+npcs.all()[nearest].title:"";
    bool changed=vendor!=nearest||identity!=nextIdentity;
    if(changed){vendor=nearest;identity=nextIdentity;stock=std::move(next);lessons=vendor>=0?teaching(npcs.all()[vendor]):std::vector<Skill>{};profession=vendor>=0?npcs.all()[vendor].title:"";selectedType=-1;title=vendor>=0?npcs.all()[vendor].name:"SIN COMERCIANTE";message=vendor>=0?"COMERCIO Y ENTRENAMIENTO":"ACERCATE A UN VENDEDOR";}
    return changed;
}
void Commerce::tap(const World&w,const Npcs&npcs,const Actor&a,Inventory&inv,int x,int y){
    update(w,npcs,a);
    if(InventoryLayout::page(x,y)){bagPage=1-bagPage;selectedType=-1;return;}
    if(!available()){message="ACERCATE A UN VENDEDOR";return;}
    int left=InventoryLayout::slot(x,y,18),right=InventoryLayout::slot(x,y,170);
    int type=-1;bool buy=right>=0;
    if(buy&&size_t(right)<stock.size())type=stock[right];
    if(left>=0&&size_t(left+bagPage*12)<inv.backpack.size())type=inv.backpack[left+bagPage*12].type;
    if(type>=0){if(!accepts(unsigned(type))){selectedType=-1;message="NO COMPRA ESTE OBJETO";return;}
        selectedType=type;buying=buy;message=std::string(buy?"COMPRAR ":"VENDER ")+itemDefs[type].name;return;}
    if(x>=170&&x<300&&y>=194&&y<210&&selectedType>0){
        // Re-evaluate proximity before every transaction; no cached vendor access.
        unsigned cost=price(unsigned(selectedType),buying);
        if(inv.trade(buying,unsigned(selectedType),cost))message=std::string(buying?"COMPRADO ":"VENDIDO ")+itemDefs[selectedType].name+(buying?" -$":" +$")+std::to_string(cost);
        else message=inv.message;
    }
}
}

namespace lab {
void TradeMenu::refresh(const Commerce&shop,const Inventory&inv){
    rows.clear();
    if(tab==0)rows=shop.stock;
    else if(tab==1){for(auto item:inv.backpack)if(item.type&&shop.accepts(item.type))rows.push_back(item.type);}
    else {for(auto skill:shop.lessons)rows.push_back(unsigned(skill));}
    row=rows.empty()?0:std::max(0,std::min(row,int(rows.size())-1));
}
void TradeMenu::switchTab(int delta,const Commerce&shop,const Inventory&inv){tab=(tab+delta+3)%3;row=0;confirming=false;message.clear();refresh(shop,inv);}
void TradeMenu::move(int delta){if(!rows.empty()){row=(row+delta+int(rows.size()))%int(rows.size());message.clear();}}

bool TradeMenu::quote(const Commerce&shop,const Inventory&inv,const PlayerStatus&status){
    refresh(shop,inv);message.clear();maximum=0;quantity=1;
    if(!shop.available()||rows.empty()){message=tab==2?"ESTE NPC NO OFRECE LECCIONES":"NO HAY OBJETOS DISPONIBLES";return false;}
    entry=rows[row];unsigned gold=PlayerStatus::gold(&inv);
    if(tab==2){auto&s=status.skills.entries[entry];unitPrice=1;
        if(!status.skills.ready||s.value>=300){message="YA ALCANZASTE EL LIMITE DE 30.0";return false;}
        if(s.lock!=SkillLock::Up){message="MARCA ESTA HABILIDAD EN SUBIR";return false;}
        maximum=std::min({300u-s.value,gold,Skills::TotalCap-status.skills.total()});
        if(!maximum){message=gold?"LIMITE TOTAL DE HABILIDADES: 700.0":"ORO INSUFICIENTE";return false;}
        quantity=maximum;
    }else{
        unitPrice=shop.price(entry,tab==0);
        if(tab==0){maximum=gold/unitPrice;if(itemDefs[entry].stackable){unsigned count=owned(inv,entry);maximum=std::min(maximum,65535u-count);}}
        else maximum=std::min(owned(inv,entry),(65535u-gold)/unitPrice);
        if(!itemDefs[entry].stackable)maximum=std::min(maximum,1u);
        if(!maximum){message=tab==0?"ORO INSUFICIENTE O PILA LLENA":"PILA DE ORO LLENA";return false;}
    }
    confirming=true;return true;
}
void TradeMenu::changeQuantity(int delta){if(confirming)quantity=unsigned(std::max(1,std::min(int(maximum),int(quantity)+delta)));}
bool TradeMenu::execute(const Commerce&shop,Inventory&inv,PlayerStatus&status,SaveRename renameFile){
    if(!confirming||!shop.available()||!quantity||quantity>maximum)return false;
    bool ok=false;
    if(tab==2){
        if(std::find(shop.lessons.begin(),shop.lessons.end(),Skill(entry))==shop.lessons.end())return false;
        auto book=status.skills;auto&s=book.entries[entry];
        if(s.lock!=SkillLock::Up||s.value>=300||s.value+quantity>300||book.total()+quantity>Skills::TotalCap){message="LECCION NO DISPONIBLE";return false;}
        s.value+=quantity;s.checks=0;s.wait=0;
        ok=inv.commitTraining(book,quantity,renameFile);if(ok){status.skills=std::move(book);status.trainingSerial=inv.trainingSerial;}
    }else if(shop.accepts(entry))ok=inv.tradeQuantity(tab==0,entry,unitPrice,quantity,renameFile);
    message=ok?(tab==2?std::string(skillDefs[entry].name)+" = "+skillValue(status.skills.entries[entry].value):(tab==0?"COMPRA":"VENTA")+std::string(" COMPLETADA: ")+std::to_string(quantity)):inv.message;
    if(ok){confirming=false;refresh(shop,inv);}return ok;
}
}
