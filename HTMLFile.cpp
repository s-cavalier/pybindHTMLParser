#include "HTMLFile.h"
#include <cctype>
#include <cstdio>
#include <forward_list>
#include <list>
#include <thread>
#include <iostream>
using namespace html;

sparser::sparser(const std::string& textref) {
    EOS = false;
    text = &textref;
    start = 0;
    len = 1;
}

void sparser::incr() {
    if (start + len + 1 >= text->size()) {
        EOS = true;
        return;
    }
    ++len;
}

void sparser::shift() {
    if (start + len + 1 >= text->size()) {
        EOS = true;
        return;
    }
    ++start;
}

void sparser::flatten() {
    start += len - 1;
    len = 1;
}

const char& sparser::back() const {
    return text->at(start + len - 1);
}

const char* sparser::windowptr() const {
    return text->c_str() + start;
}
const int& sparser::windowlen() const {
    return len;
}

bool sparser::endOfString() const {
    return EOS;
}

const bool& ComponentNode::isLocked() const {
    return ptr_lock;
}
std::vector<ComponentNode*>& ComponentNode::accessChildren() {
    if (ptr_lock) throw AttemptedLockNodeAccess(raw);
    return children;
}
ComponentNode* &ComponentNode::accessParent() {
    if (ptr_lock) throw AttemptedLockNodeAccess(raw);
    return parent;
}
std::string& ComponentNode::accessRawText() {
    if (ptr_lock) throw AttemptedLockNodeAccess("penis");
    return raw;
}
bool& ComponentNode::accessIsVoid() {
    if (ptr_lock) throw AttemptedLockNodeAccess("penis");
    
    return void_elem;    
}
const std::vector<ComponentNode*>& ComponentNode::getChildren() const {
    return children;
}
const bool& ComponentNode::isVoid() const {
    return void_elem;
}
const ComponentNode* ComponentNode::getParent() const {
    return parent;
}
const std::string& ComponentNode::getRawText() const {
    return raw;
}
const std::string& ComponentNode::getComponent() const {
    return component;
}
const std::string& ComponentNode::getText() const {
    return text;
}
const std::unordered_map<std::string, std::string>& ComponentNode::getAttributes() const {
    return attributes;
}
const std::string& ComponentNode::getId() const {
    return id;
}
const std::unordered_set<std::string>& ComponentNode::getClasses() const {
    return classes;
}
ComponentNode::ComponentNode() {
    ptr_lock = false;
    parent = nullptr;
    void_elem = false;
}

// We assume component node is a fully parsed text, i.e. it should follow the form
//<'element' attributes... >text</'element'>
//
//TODO: define a way to 'dynamically' generate the innertext s.t. the original rawtext is recreatable (as of previous, any inner node data is lose,
// i.e. <p>pe<strong>nis</strong><p>) loses the 'nis' information when trying to 'regen' the original text. Not sure if this is necessary, though.
void ComponentNode::lock() {
    // Holds current 'word', which is to say it is just some text utilized to parse the input string.
    ptr_lock = true;
    std::string word;
    // Manage opener tag
    auto it = raw.cbegin();
    ++it;

    while (std::isalnum(*it)) {
        word.push_back(*it);
        ++it;
    }
    component = word;
    word.clear();

    std::string att_value = "";
    bool inattr = false;
    // Manage attributes
    while (*it != '>') {
        if (inattr && *it != '"') {
            att_value.push_back(*it);
            ++it;
            continue;
        }
        if (inattr && *it == '"') {
            attributes[word] = att_value;
            att_value.clear();
            word.clear();
            inattr = false;
            ++it;
            continue;
        }
        if (std::isspace(*it)) {
            ++it;
            continue;
        }
        if (*it == '=') {
            ++it;
            ++it;
            inattr = true;
            continue;
        }
        
        word.push_back(*it);
        ++it;
    }

    // use the already-parsed attributes map to find classes & ids
    if (attributes.count("id")) id = attributes[id];
    if (attributes.count("class")) {
        std::string word;
        const std::string& classAttribute = attributes["class"];
        for (const auto &c : classAttribute) {
            if (std::isspace(c) && !word.empty()) {
                classes.insert(word);
                word.clear();
                continue;
            }
            word.push_back(c);
        }
        classes.insert(word);
    }

    if (void_elem) return;

    ++it;

    //Open tag is now closed; we can just enter all text directly into text until *it == '<' (close tag)
    while (*it != '<') {
        text.push_back(*it);
        ++it;
    }

    // We can ignore closing tag, literally no use for it other than to identify end
}

const int& HTMLParser::size() const {
    return _size;
}
bool HTMLParser::empty() const {
    return _size == 0;
}
const ComponentNode* HTMLParser::getRoot() const {
    if (!root) throw std::runtime_error("Root is empty");
    return root;
}

HTMLParser::HTMLParser(const std::string& input) {
    sparser sp(input);
    end_condition isnt_alnum = [] (const char& c) { return !bool(std::isalnum(c)); };
    end_condition end_of_tag = [] (const char& c) { return c == '>'; };

    std::forward_list<ComponentNode*> stack;
    for (sp; !sp.endOfString(); sp.shift()) {
        
        // Encountered tag
        if (sp.back() == '<') {
            sp.incr();
            sp.append_until(isnt_alnum);
            
            // Is a closing tag, </...>
            if (sp.getWindow()[1] == '/') {
                sp.incr();
                sp.append_until(isnt_alnum);
                stack.front()->accessRawText() += sp;
                if (++stack.begin() == stack.end()) stack.front()->lock();
                else {
                    std::thread asyncLock(&ComponentNode::lock, stack.front());
                    asyncLock.detach();
                }
                stack.pop_front();
                sp.flatten();
                continue;
            }

            if (sp.getWindow()[1] == '!') {
                sp.incr();
                sp.append_until(end_of_tag);
                sp.flatten();
                continue;
            }

            // sp should look like <tag> or <tag , so we cutoff start and end and check if void elem
            std::string tag(++sp.getWindow().cbegin(), std::prev(sp.getWindow().cend()));
            if (void_elements.count(tag)) {
                sp.append_until(end_of_tag);
                ComponentNode* newNode = new ComponentNode();
                newNode->accessRawText() = sp;
                newNode->accessIsVoid() = true;
                newNode->accessParent() = stack.front();
                // we only care to wait on the lock to finish iff lock is the root since we need the root to access all other nodes, otherwise they can be done asynchronously
                if (++stack.begin() == stack.end()) newNode->lock();
                else {
                    std::thread asyncLock(&ComponentNode::lock, newNode);
                    asyncLock.detach();
                }
                stack.front()->accessChildren().push_back(newNode);
                sp.flatten();
                continue;
            }

            // Style and Script tag text have A LOT of bad characters that can cause us to throw errors
            // since they're garaunteed to have no children and don't have any useful information, we just skip them entirely
            if (tag == "style" || tag == "script") {
                sp.flatten();
                sp.incr();
                // sp should be of size two, shift until sp == "</"
                std::string kill("</");
                while (!sp.compLast(kill)) {
                    sp.incr();
                }
                sp.incr();
                sp.append_until(end_of_tag);
                sp.flatten();
                continue;
            }

            // is an opening tag, push to stack
            ComponentNode* newNode = new ComponentNode();
            newNode->accessRawText() += sp;
            sp.flatten();
            ComponentNode* push = newNode;
            if (stack.empty()) root = newNode;
            else {
                newNode->accessParent() = stack.front();
                stack.front()->accessChildren().push_back(newNode);
            }
            stack.push_front(push);
            continue;
        }

        if (stack.empty()) continue;
        if (sp.back() != ' ' && std::isspace(sp.back())) continue;

        stack.front()->accessRawText().push_back(sp.back());

    }
}

// DESTRUCTORS
// pybind11 likes to take ownership of all pointers, so writing our own destructor cases it to segfault
// only use if compiling with C++

#ifdef CXXCOMPILED
HTMLParser::~HTMLParser() {
    if (!root) return;

    std::list<html::ComponentNode*> queue;
    queue.push_back(root);

    while (!queue.empty()) {
        html::ComponentNode* top = queue.front();
        queue.pop_front();

        for (const auto& child : top->getChildren()) queue.push_back(child);

        delete top;
    }

}

ComponentNode::~ComponentNode() {}
#endif