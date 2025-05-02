#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/LogAnywhere.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream> // for std::cerr

using namespace LogAnywhere;

// Tags for all our channels
static Tag TAG_DEFAULT("DEFAULT");
static Tag TAG_SERIAL("SERIAL");
static Tag TAG_FILE("FILE");
static Tag TAG_ASYNC("ASYNC");
static Tag TAG_TEST("TEST");
static Tag TAG_MATCH("MATCH");
static Tag TAG_NO_MATCH("NO_MATCH");
static Tag TAG_OTHER("OTHER");

// A simple handler that counts the number of times it is invoked.
static void countingHandler(const LogMessage &msg, void *ctx)
{
    auto *count = static_cast<int *>(ctx);
    ++(*count);
}

// A no-op handler for registration
static void dummyHandler(const LogMessage &, void *) {}

/**
 * @brief Clears both the HandlerManager and resets each Tag's subscriber list.
 */
inline void clearAndVerify()
{
    handlerManager.clearHandlers();

    // reset every Tag's subscriber list
    TAG_DEFAULT.handlerCount = 0;
    TAG_SERIAL.handlerCount = 0;
    TAG_FILE.handlerCount = 0;
    TAG_ASYNC.handlerCount = 0;
    TAG_TEST.handlerCount = 0;
    TAG_MATCH.handlerCount = 0;
    TAG_NO_MATCH.handlerCount = 0;

    size_t count = 0;
    handlerManager.listHandlers(count);
    REQUIRE(count == 0); // ✅ Confirm no handlers left
}

// Tests HandlerManager::clearHandlers behavior:
// 1) “registry is empty afterwards” — all handlers are removed, pointer remains valid
// 2) “ID counter resets to 1” — new registrations restart at ID 1
// 3) “does not prune Tag subscriptions” — tags retain their subscriber lists
TEST_CASE("HandlerManager::clearHandlers behaves correctly",
          "[HandlerManager][clearHandlers]")
{
    // Reset both the HandlerManager and all Tag subscription lists
    clearAndVerify();

    // Use the DEFAULT tag for all subscriptions in this test
    const Tag *tags[] = {&TAG_DEFAULT};
    // SECTION 1: “registry is empty afterwards” — all handlers are removed, pointer remains valid
    SECTION("registry is empty afterwards")
    {
        // Register two handlers to populate the registry
        handlerManager.registerHandlerForTags(LogLevel::INFO, dummyHandler, nullptr, tags, 1);
        handlerManager.registerHandlerForTags(LogLevel::WARN, dummyHandler, nullptr, tags, 1);
        REQUIRE(TAG_DEFAULT.handlerCount == 2); // verify both subscriptions added

        // Clear the registry
        handlerManager.clearHandlers();

        // Registry should report zero handlers, but pointer must remain non-null
        size_t count = 0;
        auto ptr = handlerManager.listHandlers(count);
        REQUIRE(count == 0);
        REQUIRE(ptr != nullptr);
    }

    // SECTION 2: “ID counter resets to 1” — new registrations restart at ID 1
    SECTION("ID counter resets to 1")
    {
        // Clear before registering to test that the next ID starts at 1
        handlerManager.clearHandlers();
        handlerManager.registerHandlerForTags(LogLevel::ERR, dummyHandler, nullptr, tags, 1);

        // New handler ID should be 1
        size_t count = 0;
        auto ptr = handlerManager.listHandlers(count);
        REQUIRE(count == 1);
        REQUIRE(ptr[0].id == 1);
    }

    // SECTION 3: “does not prune Tag subscriptions” — tags retain their subscriber lists
    SECTION("does not prune Tag subscriptions")
    {
        // Subscribe one handler to the DEFAULT tag
        handlerManager.registerHandlerForTags(LogLevel::INFO, dummyHandler, nullptr, tags, 1);
        REQUIRE(TAG_DEFAULT.handlerCount == 1); // subscription present

        // Clear the registry (tags are unaffected)
        handlerManager.clearHandlers();
        REQUIRE(TAG_DEFAULT.handlerCount == 1); // tag subscription remains
    }
}

// Tests HandlerManager::listHandlers behavior:
// 1) “initially empty” — registry reports zero handlers and returns a valid pointer
// 2) “after registrations returns all handlers in order” — entries appear in insertion order with correct names and sequential IDs
TEST_CASE("HandlerManager::listHandlers returns correct handler list",
          "[HandlerManager][listHandlers]")
{
    // SECTION 1: “initially empty” — registry reports zero handlers and returns a valid pointer
    SECTION("initially empty")
    {
        HandlerManager mgr;

        // Use a non-zero sentinel so we know listHandlers actually writes to 'count'
        size_t count = 12345;
        const HandlerEntry *entries = mgr.listHandlers(count);

        // Expect no handlers and a valid (non-null) pointer
        REQUIRE(count == 0);
        REQUIRE(entries != nullptr);
    }

    // SECTION 2: “after registrations returns all handlers in order” — entries appear in insertion order with correct names and sequential IDs
    SECTION("after registrations returns all handlers in order")
    {
        HandlerManager mgr;

        // Prepare two distinct tags and reset any leftover subscriptions
        Tag T1("L1"), T2("L2");
        T1.handlerCount = 0;
        T2.handlerCount = 0;
        const Tag *tags[] = {&T1, &T2};

        // Register one handler under T1, another under both T1 and T2
        mgr.registerHandlerForTags(LogLevel::INFO, dummyHandler, nullptr, tags, 1, "First");
        mgr.registerHandlerForTags(LogLevel::WARN, dummyHandler, nullptr, tags, 2, "Second");

        // List the handlers
        size_t count = 0;
        const HandlerEntry *entries = mgr.listHandlers(count);

        // Should see exactly two entries
        REQUIRE(count == 2);

        // Verify each entry’s name is non-null and matches what we registered
        REQUIRE(entries[0].name != nullptr);
        REQUIRE(std::string(entries[0].name) == "First");
        REQUIRE(entries[1].name != nullptr);
        REQUIRE(std::string(entries[1].name) == "Second");

        // IDs should be sequential (First’s ID + 1 == Second’s ID)
        REQUIRE(entries[0].id + 1 == entries[1].id);
    }
}

// Tests HandlerManager::registerHandlerForTags for:
// 1) successful registration and subscription to all tags,
// 2) skipping tag subscription when a tag is at max capacity,
// 3) returning false when the manager’s handler capacity is exceeded.
TEST_CASE("HandlerManager::registerHandlerForTags behaves correctly",
          "[HandlerManager][registerHandlerForTags]")
{
    // Each section starts with a fresh manager and tags
    HandlerManager mgr;
    std::string ctx;
    Tag T1("T1"), T2("T2");
    T1.handlerCount = 0;
    T2.handlerCount = 0;
    const Tag *tags[] = {&T1, &T2};

    // SECTION 1: “succeeds and subscribes to all specified tags”
    SECTION("succeeds and subscribes to all specified tags")
    {
        // Register a handler under both T1 and T2
        bool ok = mgr.registerHandlerForTags(LogLevel::DEBUG,
                                             dummyHandler,
                                             &ctx,
                                             tags,
                                             2,
                                             "TestHandler");
        REQUIRE(ok); // should succeed when under capacity

        // After registration, the registry contains exactly one entry
        size_t count = 0;
        auto entries = mgr.listHandlers(count);
        REQUIRE(count == 1);

        // Verify the entry’s name and that each tag gained a subscription
        REQUIRE(entries[0].name != nullptr);
        REQUIRE(std::string(entries[0].name) == "TestHandler");
        REQUIRE(T1.handlerCount == 1);
        REQUIRE(T2.handlerCount == 1);
    }

    // SECTION 2: “skips subscription when a Tag is already full”
    SECTION("skips subscription when a Tag is already full")
    {
        // Set the maximum subscription count for each Tag to 2
        T1.handlerCount = MAX_TAG_SUBSCRIPTIONS;
        T2.handlerCount = MAX_TAG_SUBSCRIPTIONS;

        // Manager capacity is not yet exceeded, so this returns true
        REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO,
                                           dummyHandler,
                                           &ctx,
                                           tags,
                                           2,
                                           "SkipSubs"));

        // Manager got the new entry…
        size_t cnt = 0;
        mgr.listHandlers(cnt);
        REQUIRE(cnt == 1);

        // …but neither Tag’s handlerCount grew beyond its max
        REQUIRE(T1.handlerCount == MAX_TAG_SUBSCRIPTIONS);
        REQUIRE(T2.handlerCount == MAX_TAG_SUBSCRIPTIONS);
    }

    // SECTION 3: “fails when capacity is exceeded”
    SECTION("fails when capacity is exceeded")
    {
        // Saturate the manager up to its maximum handlers
        for (size_t i = 0; i < LOGANYWHERE_MAX_HANDLERS; ++i)
        {
            REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO,
                                               dummyHandler,
                                               &ctx,
                                               tags,
                                               2,
                                               nullptr));
        }

        // Any further registration should be rejected
        REQUIRE_FALSE(mgr.registerHandlerForTags(LogLevel::INFO,
                                                 dummyHandler,
                                                 &ctx,
                                                 tags,
                                                 2,
                                                 nullptr));
    }
}

// Covers primary behaviors of deleteHandlerByID via the public API:
// 1) removing a registered handler,
// 2) refusing to delete a non-existent ID,
// 3) pruning exactly one subscription when multiple handlers share a tag,
// 4) exercising compactHandlerArray’s no-shift path by deleting the last entry.
TEST_CASE("HandlerManager::deleteHandlerByID covers all scenarios",
          "[HandlerManager][deleteHandlerByID]")
{
    // Each SECTION starts with a fresh manager and tag
    HandlerManager mgr;
    Tag T("DELETE_ID");
    const Tag *tags[] = {&T};

    // SECTION 1: “removes handler and unsubscribes from Tag”
    SECTION("removes handler and unsubscribes from Tag")
    {
        // Register one handler, then delete it
        mgr.registerHandlerForTags(LogLevel::INFO, dummyHandler, nullptr, tags, 1, "ToRemove");
        REQUIRE(T.handlerCount == 1);

        size_t count = 0;
        auto entries = mgr.listHandlers(count);
        uint16_t id = entries[0].id;

        REQUIRE(mgr.deleteHandlerByID(id));

        // Registry is empty and tag list cleared
        mgr.listHandlers(count);
        REQUIRE(count == 0);
        REQUIRE(T.handlerCount == 0);
    }

    // SECTION 2: “refuses to delete a non-existent ID”
    SECTION("returns false for non-existent ID")
    {
        // Register one to confirm registry isn’t empty
        mgr.registerHandlerForTags(LogLevel::INFO, dummyHandler, nullptr, tags, 1, "ToRemove");
        REQUIRE(T.handlerCount == 1);

        // Attempting to delete an ID never assigned
        REQUIRE_FALSE(mgr.deleteHandlerByID(0xFFFF));

        // Registry and tag subscription remain unchanged
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 1);
        REQUIRE(T.handlerCount == 1);
    }

    // SECTION 3: “prunes exactly one subscription when multiple handlers share a tag”
    SECTION("only removes matching subscription, leaves others intact")
    {
        // Two handlers on the same tag → delete first → one remains
        int ctxA = 0, ctxB = 0;
        REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO, countingHandler, &ctxA, tags, 1, "A"));
        REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO, countingHandler, &ctxB, tags, 1, "B"));
        REQUIRE(T.handlerCount == 2);

        size_t count = 0;
        auto entries = mgr.listHandlers(count);
        uint16_t idA = entries[0].id;

        REQUIRE(mgr.deleteHandlerByID(idA));

        // Only the second handler’s subscription remains
        REQUIRE(T.handlerCount == 1);
        mgr.listHandlers(count);
        REQUIRE(count == 1);
        REQUIRE(std::string(mgr.listHandlers(count)[0].name) == "B");
    }

    // SECTION 4: “exercises compactHandlerArray’s no-shift path by deleting the last entry”
    SECTION("deleting last entry does not shift array in compactHandlerArray")
    {
        // Two handlers → delete the second (last) → registry shrinks without shifting
        int ctx = 0;
        REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO, countingHandler, &ctx, tags, 1, "First"));
        REQUIRE(mgr.registerHandlerForTags(LogLevel::INFO, countingHandler, &ctx, tags, 1, "Second"));
        REQUIRE(T.handlerCount == 2);

        size_t count = 0;
        auto entries = mgr.listHandlers(count);
        uint16_t idSecond = entries[1].id;

        REQUIRE(mgr.deleteHandlerByID(idSecond));

        // Only the first handler remains
        mgr.listHandlers(count);
        REQUIRE(count == 1);
        REQUIRE(std::string(mgr.listHandlers(count)[0].name) == "First");
    }
}

// Tests HandlerManager::deleteHandlerByName for three distinct behaviors via the public API:
// 1) removing a named handler and unsubscribing it,
// 2) ignoring handlers registered without a name,
// 3) refusing to delete a name that was never registered.
TEST_CASE("HandlerManager::deleteHandlerByName covers all scenarios",
          "[HandlerManager][deleteHandlerByName]")
{
    // A fresh manager and an empty Tag for each SECTION
    HandlerManager mgr;
    std::string ctx;
    Tag T("DELETE_NAME");
    T.handlerCount = 0;
    const Tag *tags[] = {&T};

    // SECTION 1: “removes handler and unsubscribes from Tag”
    SECTION("removes handler and unsubscribes from Tag")
    {
        // Register one handler under "TargetHandler"
        mgr.registerHandlerForTags(LogLevel::WARN,
                                   dummyHandler,
                                   &ctx,
                                   tags,
                                   1,
                                   "TargetHandler");
        REQUIRE(T.handlerCount == 1);

        // Deleting by that name should succeed
        REQUIRE(mgr.deleteHandlerByName("TargetHandler"));

        // Registry is empty and Tag list pruned
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 0);
        REQUIRE(T.handlerCount == 0);
    }

    // SECTION 2: “ignores handlers registered without a name”
    SECTION("ignores handlers registered without a name")
    {
        // Register an unnamed handler (nullptr name) under T
        mgr.registerHandlerForTags(LogLevel::INFO,
                                   dummyHandler,
                                   &ctx,
                                   tags,
                                   1,
                                   nullptr);
        // Subscription count should go up even without a name
        REQUIRE(T.handlerCount == 1);

        // deleteHandlerByName should skip over unnamed handlers and return false
        bool result = mgr.deleteHandlerByName("anything");
        REQUIRE_FALSE(result);

        // Registry remains with exactly that unnamed entry
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 1);

        // Tag subscription is still in place
        REQUIRE(T.handlerCount == 1);
    }

    // SECTION 3: “refuses to delete a name that was never registered”
    SECTION("returns false if name not found")
    {
        // Without any registrations, deleting "NoSuch" must fail
        REQUIRE_FALSE(mgr.deleteHandlerByName("NoSuch"));

        // Registry remains empty and Tag untouched
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 0);
        REQUIRE(T.handlerCount == 0);
    }
}

// Confirms find‐by‐ID behavior through the public API:
// 1) listing returns the correct entry by ID and name,
// 2) deleteHandlerByID succeeds for a valid ID,
// 3) deleteHandlerByID fails for an invalid ID.
TEST_CASE("HandlerManager::findEntryByID via public API",
          "[HandlerManager][findEntryByID]")
{
    // Reset the global manager and any tag subscriptions (if you’re using the globals)
    clearAndVerify();

    // Use a fresh, local Tag so its handlerCount starts at 0 each time
    Tag T("DEFAULT");
    T.handlerCount = 0;
    const Tag *tags[] = {&T};

    HandlerManager mgr;

    // Register exactly one handler and capture its ID
    mgr.registerHandlerForTags(LogLevel::INFO,
                               dummyHandler,
                               nullptr,
                               tags,
                               1,
                               "LookupTest");

    // SECTION 1: Verify listHandlers actually exposes our entry
    SECTION("listHandlers returns entry with correct ID and name")
    {
        size_t cnt = 0;
        auto entries = mgr.listHandlers(cnt);
        REQUIRE(cnt == 1);

        // Find our handler in the returned array
        bool found = false;
        for (size_t i = 0; i < cnt; ++i)
        {
            if (entries[i].id == entries[0].id && entries[i].name && std::string(entries[i].name) == "LookupTest")
            {
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    // SECTION 2: Deleting by that valid ID should succeed and empty the registry
    SECTION("deleteHandlerByID removes existing handler")
    {
        // First re-fetch the ID (in case sections run out of order)
        size_t cnt = 0;
        auto entries = mgr.listHandlers(cnt);
        uint16_t validID = entries[0].id;

        REQUIRE(mgr.deleteHandlerByID(validID));

        size_t after = 0;
        mgr.listHandlers(after);
        REQUIRE(after == 0);
    }

    // SECTION 3: Deleting a non-existent ID should simply return false
    SECTION("deleteHandlerByID returns false for non-existent ID")
    {
        // Use an ID we never assigned
        REQUIRE_FALSE(mgr.deleteHandlerByID(0xFFFF));
    }
}

// Verifies name‐based lookup and deletion via the public API:
// 1) listHandlers exposes the entry with the correct name,
// 2) deleteHandlerByName removes it when the name exists,
// 3) deleteHandlerByName returns false when the name does not exist.
TEST_CASE("HandlerManager::findEntryByName via public API",
          "[HandlerManager][findEntryByName]")
{
    // Ensure a clean global state if other tests use the globals
    clearAndVerify();

    // Use a local Tag so its handlerCount starts at 0 each section
    Tag T("DEFAULT");
    T.handlerCount = 0;
    const Tag *tags[] = {&T};

    HandlerManager mgr;
    std::string ctx;

    // Register a single handler named "NameTest"
    mgr.registerHandlerForTags(LogLevel::DEBUG,
                               dummyHandler,
                               &ctx,
                               tags,
                               1,
                               "NameTest");
    // SECTION 1: listHandlers exposes the entry with the correct name,
    SECTION("listHandlers returns entry with the given name")
    {
        // List current handlers
        size_t count = 0;
        auto entries = mgr.listHandlers(count);
        REQUIRE(count == 1); // one entry registered

        // Verify the name matches exactly
        REQUIRE(entries[0].name != nullptr);
        REQUIRE(std::string(entries[0].name) == "NameTest");
    }

    // SECTION 2: deleteHandlerByName removes it when the name exists,
    SECTION("deleteHandlerByName succeeds for existing name")
    {
        // Should remove the handler we just registered
        REQUIRE(mgr.deleteHandlerByName("NameTest"));

        // After deletion, registry must be empty
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 0);
    }

    // SECTION 3: deleteHandlerByName returns false when the name does not exist.
    SECTION("deleteHandlerByName returns false for unknown name")
    {
        // Attempting to delete a name never registered should fail
        REQUIRE_FALSE(mgr.deleteHandlerByName("NoSuchName"));

        // Registry remains unchanged
        size_t count = 0;
        mgr.listHandlers(count);
        REQUIRE(count == 1);
    }
}
