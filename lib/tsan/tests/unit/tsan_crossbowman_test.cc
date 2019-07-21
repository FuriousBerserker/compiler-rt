#include "tsan_crossbowman.h"
#include "gtest/gtest.h"
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

namespace __tsan {
TEST(Crossbowman, IntervalTreeIterator) {
    Interval i{1, 10};
    MapInfo m{1, 9};
    IntervalTree tree{};
    tree.insert(i, m);
    auto &&it = tree.begin();
    auto &&ie = tree.end();
    ASSERT_NE(it, ie);
    Node *n = *it;
    ASSERT_EQ(n->interval, i);
    ++it;
    ASSERT_EQ(it, ie);
}

TEST(Crossbowman, IntervalTreeInsert) {
    vector<Interval> ints{{5, 10}, {1, 3}, {10, 15}, {15, 20}, {3, 5}};
    vector<MapInfo> maps{{5, 5}, {1, 2}, {10, 5}, {15, 5}, {3, 2}}; 

    IntervalTree tree{};
    unsigned i = 0;
    for (; i < ints.size(); i++) {
        tree.insert(ints[i], maps[i]);
    }
    sort(ints.begin(), ints.end());
    i = 0;
    for (auto &&it : tree) {
        EXPECT_EQ(it->interval, ints[i++]);
    }
}

TEST(Crossbowman, IntervalTreeDeleteLeafNode) {
    vector<Interval> ints{{5, 10}, {1, 3}, {10, 15}, {15, 20}, {3, 5}};
    vector<Interval> exps{{5, 10}, {1, 3}, {10, 15}, {3, 5}};
    vector<MapInfo> maps{{5, 5}, {1, 2}, {10, 5}, {15, 5}, {3, 2}}; 

    IntervalTree tree{};
    unsigned i = 0;
    for (; i < ints.size(); i++) {
        tree.insert(ints[i], maps[i]);
    }
    tree.remove(ints[3]);
    sort(exps.begin(), exps.end());
    i = 0;
    for (auto &&it : tree) {
        //cout << it->interval.left_end << ", " << it->interval.right_end << endl; 
        EXPECT_EQ(it->interval, exps[i++]);
    }
}


TEST(Crossbowman, IntervalTreeDeleteInternalNode1) {
    vector<Interval> ints{{5, 10}, {1, 3}, {10, 15}, {15, 20}, {3, 5}};
    vector<Interval> exps{{5, 10}, {10, 15}, {15, 20}, {3, 5}};
    vector<MapInfo> maps{{5, 5}, {1, 2}, {10, 5}, {15, 5}, {3, 2}}; 

    IntervalTree tree{};
    unsigned i = 0;
    for (; i < ints.size(); i++) {
        tree.insert(ints[i], maps[i]);
        //for (auto &&it : tree) {
            //cout << it << " " << it->interval.left_end << ", " << it->interval.right_end << ", " << it->left_child << ", " << it->right_child << ", " << it->parent << endl; 
        //} 
    }

    tree.remove(ints[1]);

    sort(exps.begin(), exps.end());
    i = 0;
    for (auto &&it : tree) {
        EXPECT_EQ(it->interval, exps[i++]);
    }
}

TEST(Crossbowman, IntervalTreeDeleteInternalNode2) {
    vector<Interval> ints{{5, 10}, {1, 3}, {10, 15}, {15, 20}, {3, 5}};
    vector<Interval> exps{{1, 3}, {10, 15}, {15, 20}, {3, 5}};
    vector<MapInfo> maps{{5, 5}, {1, 2}, {10, 5}, {15, 5}, {3, 2}}; 

    IntervalTree tree{};
    unsigned i = 0;
    for (; i < ints.size(); i++) {
        tree.insert(ints[i], maps[i]);
    }
    tree.remove(ints[0]);
    sort(exps.begin(), exps.end());
    i = 0;
    for (auto &&it : tree) {
        EXPECT_EQ(it->interval, exps[i++]);
    }

    EXPECT_EQ(tree.getRoot()->interval, (Interval{3, 5}));
}

} // namespace __tsan
