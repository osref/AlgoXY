#ifndef _EDICT_
#define _EDICT_

#include <list>
#include <queue>
#include <algorithm> //for std::equal
#include "trie.hpp"
#include "patricia.hpp"
#include "boost/tuple/tuple.hpp"

template<class T>
std::list<std::pair<typename T::KeyType, typename T::ValueType> >
expand(typename T::KeyType prefix, T* t, unsigned int n)
{
  typedef typename T::KeyType KeyType;
  typedef typename T::ValueType ValueType;
  typedef std::list<std::pair<KeyType, ValueType> > Result;

  Result res;
  std::queue<std::pair<KeyType, T*> > q;
  q.push(std::make_pair(prefix, t));
  while(res.size()<n && (!q.empty())){
    std::pair<KeyType, T*> i = q.front();
    KeyType s = i.first;
    T* p = i.second;
    q.pop();
    if(p->value != ValueType()){
      res.push_back(std::make_pair(s, p->value));
    }
    for(typename T::Children::iterator it = p->children.begin();
        it!=p->children.end(); ++it)
      q.push(std::make_pair(s+it->first, it->second));
  }
  return res;
}

//lookup top n candidate with prefix key in Trie
template<class K, class V>
std::list<std::pair<K, V> > lookup(Trie<K, V>* t, 
                                   typename Trie<K, V>::KeyType key, 
                                   unsigned int n)
{
  typedef std::list<std::pair<K, V> > Result;
  if(!t)
    return Result();

  Trie<K, V>* p(t);
  for(typename K::iterator it=key.begin(); it!=key.end(); ++it){
    if(p->children.find(*it) == p->children.end())
      return Result();
    p = p->children[*it];
  }
  return expand(key, p, n);
}

// x `is prefix of` y?
template<class T>
bool is_prefix_of(T x, T y){
  if(x.size()<=y.size())
    return std::equal(x.begin(), x.end(), y.begin());
  return false;
}

template<class K, class V>
std::list<std::pair<K, V> > lookup(Patricia<K, V>* t, 
                                   typename Patricia<K, V>::KeyType key,
                                   unsigned int n)
{
  typedef typename std::list<std::pair<K, V> > Result;
  typedef typename Patricia<K, V>::Children::iterator Iterator;
  if(!t)
    return Result();
  K prefix;
  for(;;){
    bool match(false);
    for(Iterator it=t->children.begin(); it!=t->children.end(); ++it){
      K k(it->first);
      if(is_prefix_of(key, k))
        return expand(prefix+k, it->second, n);
      if(is_prefix_of(k, key)){
        match = true;
        prefix += k;
        lcp<K>(key, k); //update key
        t = it->second;
        break;
      }
    }
    if(!match)
      return Result();
  }
}

struct t9map{
  std::map<char, std::string> map;
  t9map(){
    map['2']="abc";
    map['3']="def";
    map['4']="ghi";
    map['5']="jkl";
    map['6']="mno";
    map['7']="pqrs";
    map['8']="tuv";
    map['9']="wxyz";
  }

  static t9map& inst(){
    static t9map i;
    return i;
  }
};

template<class K, class V>
std::list<std::pair<K, V> > lookup_t9(Trie<K, V>* t,
				   typename Trie<K, V>::KeyType key)
{
  typedef std::list<std::pair<K, V> > Result;
  typedef typename Trie<K, V>::KeyType Key;
  typedef typename Trie<K, V>::Char Char;

  if((!t) || key.empty())
    return Result();
  
  Key prefix;
  std::map<Char, Key> m = t9map::inst().map;
  std::queue<boost::tuple<Key, Key, Trie<K, V>*> > q;
  q.push(boost::make_tuple(prefix, key, t));
  Result res;
  while(!q.empty()){
    boost::tie(prefix, key, t) = q.front();
    q.pop();
    Char c = *key.begin();
    key = Key(key.begin()+1, key.end());
    if(m.find(c) == m.end())
      return Result();
    Key cs = m[c];
    for(typename Key::iterator i=cs.begin(); i!=cs.end(); ++i)
      if(t->children.find(*i)!=t->children.end()){
	if(key.empty())
	  res.push_back(std::make_pair(prefix+*i, t->children[*i]->value));
	else
	  q.push(boost::make_tuple(prefix+*i, key, t->children[*i]));
      }
  }
  return res;
}

//list of pairs to string
template<class Container>
std::string lop_to_str(Container coll){
  typedef typename Container::iterator Iterator;
  std::ostringstream s;
  s<<"[";
  for(Iterator it=coll.begin(); it!=coll.end(); ++it)
    s<<"("<<it->first<<", "<<it->second<<"), ";
  s<<"]";
  return s.str();
}

class EDictTest{
public:
  EDictTest():t(0), p(0), t9trie(0), t9patricia(0){
    const char* dict[] = {
      "a", "the first letter of English",                               \
      "an", "used instead of 'a' when the following word begins with a vowel sound", \
      "another", "one more person or thing or an extra amount",          \
      "abandon", "to leave a place, thing or person forever",            \
      "about", "on the subject of; connected with",                      \
      "adam", "a character in the Bible who was the first man made by God", \
      "boy", "a male child or, more generally, a male of any age",       \
      "body", "the whole physical structure that forms a person or animal", \
      "zoo", "an area in which animals, especially wild animals, are kept so that people can go and look at them, or study them"};

    const char** first=dict;
    const char** last =dict + sizeof(dict)/sizeof(char*);
    for(;first!=last; ++first, ++first){
      t = insert(t, *first, *(first+1));
      p = insert(p, *first, *(first+1));
    }

    const char* t9dict[] = {"home", "good", "gone", "hood", "a", "another", "an"};
    t9trie = list_to_trie(t9dict, t9dict+sizeof(t9dict)/sizeof(char*), t9trie);
    t9patricia = list_to_trie(t9dict, t9dict+sizeof(t9dict)/sizeof(char*), t9patricia);
  }

  ~EDictTest(){
    delete t;
    delete p;
    delete t9trie;
    delete t9patricia;
  }

  void run(){
    std::cout<<"\nword auto-completion and T9 test\n";
    test_trie_lookup();
    test_patricia_lookup();
    test_trie_t9();
  }

private:
  void test_trie_lookup(){
    std::cout<<"test lookup top 5 in Trie\n"
             <<"search a "<<lop_to_str(lookup(t, "a", 5))<<"\n"
             <<"search ab "<<lop_to_str(lookup(t, "ab", 5))<<"\n\n";
  }

  void test_patricia_lookup(){
    std::cout<<"test lookup top 5 in Patricia\n"
             <<"search a "<<lop_to_str(lookup(p, "a", 5))<<"\n"
             <<"search ab "<<lop_to_str(lookup(p, "ab", 5))<<"\n\n";
  }

  void test_trie_t9(){
    std::cout<<"\ntest t9 lookup in Trie\n"
	     <<"search 4 "<<lop_to_str(lookup_t9(t9trie, "4"))<<"\n"
	     <<"serach 46 "<<lop_to_str(lookup_t9(t9trie, "46"))<<"\n"
	     <<"serach 4663 "<<lop_to_str(lookup_t9(t9trie, "4663"))<<"\n"
	     <<"serach 2 "<<lop_to_str(lookup_t9(t9trie, "2"))<<"\n"
	     <<"serach 22 "<<lop_to_str(lookup_t9(t9trie, "22"))<<"\n\n";
  }

  Trie<std::string, std::string>* t;
  Patricia<std::string, std::string>* p;
  Trie<std::string, std::string>* t9trie;
  Patricia<std::string, std::string>* t9patricia;
};

#endif //_EDICT_
