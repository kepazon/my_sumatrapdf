/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

class SquareTreeNode {
public:
    SquareTreeNode() { }
    ~SquareTreeNode();

    struct DataItem {
        const char *key;
        union {
            const char *str;
            SquareTreeNode *child;
        } value;
        bool isChild;

        DataItem() : key(nullptr) { }
        DataItem(const char *key, const char *string) : key(key), isChild(false) { value.str = string; }
        DataItem(const char *key, SquareTreeNode *node) : key(key), isChild(true) { value.child = node; }
    };
    Vec<DataItem> data;

    const char *GetValue(const char *key, size_t *startIdx=nullptr) const;
    SquareTreeNode *GetChild(const char *key, size_t *startIdx=nullptr) const;
};

class SquareTree {
    ScopedMem<char> dataUtf8;

public:
    explicit SquareTree(const char *data);
    ~SquareTree() { delete root; }

    SquareTreeNode *root;
};
