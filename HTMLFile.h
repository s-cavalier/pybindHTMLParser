#ifndef HTMLFILE_H
#define HTMLFILE_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <deque>
#include <stdexcept>

typedef bool (*end_condition)(const char&);

namespace html {

    inline static const std::unordered_set<std::string> void_elements = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};

    struct AttemptedLockNodeAccess : public std::runtime_error {
        AttemptedLockNodeAccess(const std::string& identifier) : std::runtime_error("Attempted to access locked node: " + identifier) { }
    };


    // To prefer the iterative solution over the recursive solution, there exists functions that allow for reference access to all inner data
    // that will be managed in the wrapper class HTMLParser. However, once the lock() function is called, any access functions will throw
    // AttemptedLockNodeAccess to prevent from writing to the file; we only want to read it.
    // Note that raw will only contain the open tag, innertext, and close tag to prevent from a lot of copying.
    class ComponentNode {
        // Pointers
        std::vector<ComponentNode*> children;
        ComponentNode* parent;

        bool ptr_lock;
        bool void_elem;

        // Data
        std::string raw;
        // Below is undefined and should not be access until close() is called, as we don't have all info until we encounter the closing tag.
        std::string component;
        std::string text;
        std::unordered_map<std::string, std::string> attributes;
        std::string id;
        std::unordered_set<std::string> classes;

    public:
        ComponentNode();

        #ifdef CXXCOMPILED
        ~ComponentNode();
        #endif

        // Pointer access
        const bool& isLocked() const;
        std::vector<ComponentNode*>& accessChildren();
        ComponentNode* &accessParent();
        std::string& accessRawText();
        bool& accessIsVoid();

        // ALL ComponentNodes should be locked once they are fully constructed; the specific branch upon which this node lies upon is closed.
        void lock();

        // Data
        const std::vector<ComponentNode*>& getChildren() const;
        const ComponentNode* getParent() const;

        const bool& isVoid() const;
        const std::string& getRawText() const;
        const std::string& getComponent() const;
        const std::string& getText() const;
        const std::unordered_map<std::string, std::string>& getAttributes() const;
        const std::string& getId() const;
        const std::unordered_set<std::string>& getClasses() const;
    };

    class sparser {
        const std::string* text;
        int start;
        int len;
        bool EOS;
    
    public:
        sparser(const std::string& textref);
        void incr();
        void shift();
        void flatten();
        void append_until(end_condition kill);
        void compLast(const string& comp);
        const char& back() const;
        const char* windowptr() const;
        const int& windowlen() const;
        bool endOfString() const;
        operator std::string();
    };

    class HTMLParser {
        ComponentNode* root; // <html> component
        int _size;

    public:
        // Takes in path to HTML file to parse, converts to a graph of HTML components.
        HTMLParser(const std::string& file);

        #ifdef CXXCOMPILED
        ~HTMLParser();
        #endif

        const int& size() const;
        bool empty() const;
        const ComponentNode* getRoot() const;
    };
}

#endif