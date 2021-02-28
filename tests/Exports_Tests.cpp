#include "CppUnitTest.h"

#include "Exports.hh"

#include "RA_BuildVer.h"

#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockOverlayTheme.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockSurface.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"
#include "tests\ui\UIAsserts.hh"

#include "services\AchievementRuntime.hh"

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::mocks::MockServer;
using ra::data::context::mocks::MockEmulatorContext;
using ra::data::context::mocks::MockGameContext;
using ra::data::context::mocks::MockSessionTracker;
using ra::data::context::mocks::MockUserContext;
using ra::services::mocks::MockAudioSystem;
using ra::services::mocks::MockClock;
using ra::services::mocks::MockConfiguration;
using ra::services::mocks::MockFrameEventQueue;
using ra::services::mocks::MockThreadPool;
using ra::ui::mocks::MockDesktop;
using ra::ui::mocks::MockOverlayTheme;
using ra::ui::drawing::mocks::MockSurfaceFactory;
using ra::ui::viewmodels::MessageBoxViewModel;
using ra::ui::viewmodels::mocks::MockOverlayManager;
using ra::ui::viewmodels::mocks::MockWindowManager;

namespace ra {
namespace tests {

TEST_CLASS(Exports_Tests)
{
public:
    TEST_METHOD(TestIntegrationVersion)
    {
        Assert::AreEqual(RA_INTEGRATION_VERSION, _RA_IntegrationVersion());
    }

    TEST_METHOD(TestHostName)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual("retroachievements.org", _RA_HostName());

        mockConfiguration.SetHostName("stage.retroachievements.org");
        Assert::AreEqual("stage.retroachievements.org", _RA_HostName());
    }

    TEST_METHOD(TestHostUrl)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual("http://retroachievements.org", _RA_HostUrl());

        mockConfiguration.SetHostName("stage.retroachievements.org");
        Assert::AreEqual("http://stage.retroachievements.org", _RA_HostUrl());
    }

    TEST_METHOD(TestHardcoreModeIsActive)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(1, _RA_HardcoreModeIsActive());

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(0, _RA_HardcoreModeIsActive());
    }

private:
    class WarnDisableHardcoreHarness
    {
    public:
        MockConfiguration mockConfiguration;
        MockDesktop mockDesktop;
        MockEmulatorContext mockEmulatorContext;
    };

    TEST_METHOD(TestWarnDisableHardcoreNotEnabled)
    {
        WarnDisableHardcoreHarness harness;
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        Assert::IsTrue(_RA_WarnDisableHardcore("test"));
        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestWarnDisableHardcoreAborted)
    {
        WarnDisableHardcoreHarness harness;
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        bool bWasShown = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWasShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Disable Hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"You cannot test while Hardcore mode is active."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            bWasShown = true;

            return ra::ui::DialogResult::No;
        });

        Assert::IsFalse(_RA_WarnDisableHardcore("test"));
        Assert::IsTrue(bWasShown);
        Assert::IsTrue(harness.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestWarnDisableHardcoreAllowed)
    {
        WarnDisableHardcoreHarness harness;
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        bool bWasShown = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWasShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Disable Hardcore mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"You cannot test while Hardcore mode is active."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            bWasShown = true;

            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(_RA_WarnDisableHardcore("test"));
        Assert::IsTrue(bWasShown);
        Assert::IsFalse(harness.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

private:
    class AttemptLoginHarness
    {
    public:
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockAudioSystem mockAudioSystem;
        MockServer mockServer;
        MockOverlayManager mockOverlayManager;
        MockEmulatorContext mockEmulatorContext;
        MockDesktop mockDesktop;
        MockThreadPool mockThreadPool;
        MockWindowManager mockWindowManager;
    };

public:

    TEST_METHOD(TestAttemptLoginNewUser)
    {
        AttemptLoginHarness harness;

        bool bLoginDialogShown = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bLoginDialogShown](_UNUSED ra::ui::viewmodels::LoginViewModel&)
        {
            bLoginDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        harness.mockServer.HandleRequest<api::Login>([](_UNUSED const ra::api::Login::Request&, _UNUSED ra::api::Login::Response&)
        {
            Assert::IsFalse(true, L"API called without user info");
            return false;
        });

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsTrue(bLoginDialogShown);
    }

    TEST_METHOD(TestAttemptLoginSuccess)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });
        harness.mockEmulatorContext.MockClient("RATests", "0.1.2.0");

        bool bLoggedIn = false;
        harness.mockServer.HandleRequest<api::Login>([&bLoggedIn](const ra::api::Login::Request& request, ra::api::Login::Response& response)
        {
            Assert::AreEqual(std::string("User"), request.Username);
            Assert::AreEqual(std::string("ApiToken"), request.ApiToken);
            bLoggedIn = true;

            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        // user context
        Assert::IsTrue(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string("User"), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, harness.mockUserContext.GetScore());

        // session context
        Assert::AreEqual(std::wstring(L"User"), harness.mockSessionTracker.GetUsername());

        // popup notification and sound
        Assert::IsTrue(harness.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\login.wav"));
        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome User"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"12345 points"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());

        // menu
        Assert::IsTrue(bWasMenuRebuilt);

        // titlebar
        Assert::AreEqual(std::wstring(L"RATests - 0.1 - User []"), harness.mockWindowManager.Emulator.GetWindowTitle());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithMessages)
    {
        AttemptLoginHarness harness;

        harness.mockServer.HandleRequest<api::Login>([](const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.NumUnreadMessages = 3;
            response.Score = 0U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome User"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 3 new messages"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"0 points"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithPreviousSessionData)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockServer.HandleRequest<api::Login>([](const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        harness.mockSessionTracker.MockSession(6U, 123456789, std::chrono::hours(2));

        _RA_AttemptLogin(true);

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome back User"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"12345 points"), pPopup->GetDetail());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
        Assert::IsTrue(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginInvalid)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Login Failed"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Invalid user/password combination. Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());

            return ra::ui::DialogResult::OK;
        });

        harness.mockServer.HandleRequest<api::Login>([](_UNUSED const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.ErrorMessage = "Invalid user/password combination. Please try again.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        Assert::IsTrue(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginDisabled)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        harness.mockUserContext.DisableLogin();
        _RA_AttemptLogin(true);

        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestAttemptLoginDisabledDuringRequest)
    {
        AttemptLoginHarness harness;

        bool bWasMenuRebuilt = false;
        harness.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt]() { bWasMenuRebuilt = true; });

        harness.mockConfiguration.SetUsername("User");
        harness.mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(false);

        harness.mockUserContext.DisableLogin();
        harness.mockThreadPool.ExecuteNextTask();

        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::IsFalse(harness.mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), harness.mockUserContext.GetApiToken());
        Assert::AreEqual(0U, harness.mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), harness.mockSessionTracker.GetUsername());
        Assert::IsFalse(bWasMenuRebuilt);
    }

    TEST_METHOD(TestUserNameNotLoggedIn)
    {
        MockUserContext mockUserContext;
        Assert::AreEqual(_RA_UserName(), "");
    }

    TEST_METHOD(TestUserNameLoggedIn)
    {
        MockUserContext mockUserContext;
        mockUserContext.Initialize("Test", "ABCDEFG");
        Assert::AreEqual(_RA_UserName(), "Test");
    }

private:
    class MockAchievementRuntime : public ra::services::AchievementRuntime
    {
    public:
        MockAchievementRuntime() noexcept : m_Override(this) {}

        void QueueChange(ChangeType nType, unsigned int nId, int nValue = 0)
        {
            m_vChanges.emplace_back(Change{ nType, nId, nValue });
        }

        void Process(_Inout_ std::vector<Change>& changes) noexcept override
        {
            GSL_SUPPRESS_F6
            changes.insert(changes.begin(), m_vChanges.begin(), m_vChanges.end());
            m_vChanges.clear();
        }

    private:
        std::vector<ra::services::AchievementRuntime::Change> m_vChanges;
        ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
    };

    class DoAchievementsFrameHarness
    {
    public:
        MockAchievementRuntime mockRuntime;
        MockGameContext mockGameContext;
        MockDesktop mockDesktop;
        MockOverlayManager mockOverlayManager;
        MockAudioSystem mockAudioSystem;
        MockConfiguration mockConfiguration;
        MockEmulatorContext mockEmulatorContext;
        MockSurfaceFactory mockSurfaceFactory;
        MockOverlayTheme mockTheme;
        MockFrameEventQueue mockFrameEventQueue;
        MockWindowManager mockWindowManager;
        MockClock mockClock;

        DoAchievementsFrameHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockServer.HandleRequest<ra::api::AwardAchievement>([this]
                (const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
            {
                m_vUnlockedAchievements.insert(request.AchievementId);
                response.Result = ra::api::ApiResult::Success;

                return true;
            });

            GSL_SUPPRESS_F6 mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([this]
                (const ra::api::SubmitLeaderboardEntry::Request& request, ra::api::SubmitLeaderboardEntry::Response& response)
            {
                m_vSubmittedLeaderboardEntries.insert_or_assign(request.LeaderboardId, request.Score);
                response.Result = ra::api::ApiResult::Success;

                return true;
            });
        }

        ra::data::models::AchievementModel& MockAchievement(unsigned int nId)
        {
            auto& pAch = mockGameContext.Assets().NewAchievement();
            pAch.SetCategory(ra::data::models::AssetCategory::Core);
            pAch.SetID(nId);
            pAch.SetTrigger("1=1");
            pAch.UpdateServerCheckpoint();
            pAch.SetState(ra::data::models::AssetState::Active);
            return pAch;
        }

        bool WasUnlocked(unsigned int nId)
        {
            mockThreadPool.ExecuteNextTask();
            return m_vUnlockedAchievements.find(nId) != m_vUnlockedAchievements.end();
        }

        unsigned int GetSubmittedScore(ra::LeaderboardID nId)
        {
            mockThreadPool.ExecuteNextTask();
            const auto pIter = m_vSubmittedLeaderboardEntries.find(nId);
            return (pIter != m_vSubmittedLeaderboardEntries.end()) ? pIter->second : 0U;
        }

    private:
        MockThreadPool mockThreadPool;
        MockServer mockServer;
        MockUserContext mockUserContext;

        std::set<unsigned int> m_vUnlockedAchievements;
        std::map<ra::LeaderboardID, unsigned int> m_vSubmittedLeaderboardEntries;
    };

public:
    TEST_METHOD(TestDoAchievementsFrameAchievementTriggered)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        harness.mockGameContext.SetRichPresenceDisplayString(L"Titles");

        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::wstring(L"Titles"), harness.mockGameContext.Assets().FindAchievement(1U)->GetUnlockRichPresence());
    }

    TEST_METHOD(TestDoAchievementsFrameAchievementTriggeredNoRichPresence)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);

        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
        Assert::AreEqual(std::wstring(), harness.mockGameContext.Assets().FindAchievement(1U)->GetUnlockRichPresence());
    }

    TEST_METHOD(TestDoAchievementsFrameAchievementPrimed)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U).SetBadge(L"55223");
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementPrimed, 1U);

        _RA_DoAchievementsFrame();

        const auto* pIndicator = harness.mockOverlayManager.GetChallengeIndicator(1U);
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);

        Assert::AreEqual(ra::ui::ImageType::Badge, pIndicator->GetImage().Type());
        Assert::AreEqual(std::string("55223"), pIndicator->GetImage().Name());
    }

    TEST_METHOD(TestDoAchievementsFrameAchievementUnprimed)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U).SetBadge(L"55223");
        harness.mockOverlayManager.AddChallengeIndicator(1U, ra::ui::ImageType::Badge, "55223");

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementActivated, 1U);

        _RA_DoAchievementsFrame();

        const auto* pIndicator = harness.mockOverlayManager.GetChallengeIndicator(1U);
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);

        Assert::IsTrue(pIndicator->IsDestroyPending());
    }

    TEST_METHOD(TestDoAchievementsFrameAchievementPrimedTriggered)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U).SetBadge(L"55223");
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementPrimed, 1U);

        _RA_DoAchievementsFrame();

        const auto* pIndicator = harness.mockOverlayManager.GetChallengeIndicator(1U);
        Assert::IsNotNull(pIndicator);
        Ensures(pIndicator != nullptr);
        Assert::IsFalse(pIndicator->IsDestroyPending());

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);

        _RA_DoAchievementsFrame();

        const auto* pIndicator2 = harness.mockOverlayManager.GetChallengeIndicator(1U);
        Assert::IsNotNull(pIndicator2);
        Ensures(pIndicator2 != nullptr);
        Assert::IsTrue(pIndicator2->IsDestroyPending());
    }

    TEST_METHOD(TestDoAchievementsFramePaused)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        harness.mockRuntime.SetPaused(true);

        _RA_DoAchievementsFrame();
        Assert::IsFalse(harness.WasUnlocked(1U));

        harness.mockRuntime.SetPaused(false);
        _RA_DoAchievementsFrame();
        Assert::IsTrue(harness.WasUnlocked(1U));
    }

    TEST_METHOD(TestDoAchievementsFramePauseOnTrigger)
    {
        DoAchievementsFrameHarness harness;
        auto& pAch = harness.MockAchievement(1U);
        pAch.SetName(L"AchievementTitle");
        pAch.UpdateServerCheckpoint();
        pAch.SetPauseOnTrigger(true);

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        bool bWasPaused = false;
        harness.mockEmulatorContext.SetPauseFunction([&bWasPaused] { bWasPaused = true; });

        bool bMessageSeen = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"The following triggers have triggered:\n* AchievementTitle"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        _RA_DoAchievementsFrame();
        harness.mockFrameEventQueue.DoFrame(); // UpdateUIForFrameChange isn't called in the unit test build

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsTrue(bMessageSeen);
        Assert::IsTrue(bWasPaused);
    }

    TEST_METHOD(TestDoAchievementsFramePauseOnReset)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockGameContext.Assets().FindAchievement(1U)->SetPauseOnReset(true);
        harness.mockGameContext.Assets().FindAchievement(1U)->SetName(L"AchievementTitle");
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementReset, 1U);
        bool bWasPaused = false;
        harness.mockEmulatorContext.SetPauseFunction([&bWasPaused] { bWasPaused = true; });

        bool bMessageSeen = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"The following triggers have been reset:\n* AchievementTitle"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        _RA_DoAchievementsFrame();
        harness.mockFrameEventQueue.DoFrame(); // UpdateUIForFrameChange isn't called in the unit test build

        Assert::IsTrue(bMessageSeen);
        Assert::IsTrue(bWasPaused);
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardStart)
    {
        DoAchievementsFrameHarness harness;
        harness.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted, ra::ui::viewmodels::PopupLocation::BottomLeft);
        auto& pLeaderboard = harness.mockGameContext.NewLeaderboard(1U);
        pLeaderboard.SetTitle("Title");
        pLeaderboard.SetDescription("Description");

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 1U, 1234U);
        _RA_DoAchievementsFrame();

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::IsTrue(harness.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\lb.wav"));
        Assert::AreEqual(std::wstring(L"Leaderboard attempt started"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Description"), pPopup->GetDetail());

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNotNull(pScore);
        Ensures(pScore != nullptr);
        Assert::AreEqual(std::wstring(L"1234"), pScore->GetDisplayText());
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardStartPopupDisabled)
    {
        DoAchievementsFrameHarness harness;
        harness.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardStarted, ra::ui::viewmodels::PopupLocation::None);
        harness.mockGameContext.NewLeaderboard(1U);

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 1U, 1234U);
        _RA_DoAchievementsFrame();

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNotNull(pScore);
        Ensures(pScore != nullptr);
        Assert::AreEqual(std::wstring(L"1234"), pScore->GetDisplayText());
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardCanceled)
    {
        DoAchievementsFrameHarness harness;
        harness.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled, ra::ui::viewmodels::PopupLocation::BottomLeft);
        auto& pLeaderboard = harness.mockGameContext.NewLeaderboard(1U);
        pLeaderboard.SetTitle("Title");
        pLeaderboard.SetDescription("Description");

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardCanceled, 1U);
        _RA_DoAchievementsFrame();

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::IsTrue(harness.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\lbcancel.wav"));
        Assert::AreEqual(std::wstring(L"Leaderboard attempt failed"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Title"), pPopup->GetDescription());

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNull(pScore);
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardCaneledPopupDisabled)
    {
        DoAchievementsFrameHarness harness;
        harness.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardCanceled, ra::ui::viewmodels::PopupLocation::None);
        auto& pLeaderboard = harness.mockGameContext.NewLeaderboard(1U);
        pLeaderboard.SetTitle("Title");
        pLeaderboard.SetDescription("Description");

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardCanceled, 1U);
        _RA_DoAchievementsFrame();

        const auto* pPopup = harness.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNull(pScore);
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardUpdated)
    {
        DoAchievementsFrameHarness harness;
        harness.mockGameContext.NewLeaderboard(1U);
        harness.mockOverlayManager.AddScoreTracker(1U);

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardUpdated, 1U, 1235U);
        _RA_DoAchievementsFrame();

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNotNull(pScore);
        Ensures(pScore != nullptr);
        Assert::AreEqual(std::wstring(L"1235"), pScore->GetDisplayText());
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardTriggered)
    {
        DoAchievementsFrameHarness harness;
        harness.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        harness.mockGameContext.NewLeaderboard(1U);

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardTriggered, 1U, 1236U);
        _RA_DoAchievementsFrame();

        Assert::AreEqual(1236U, harness.GetSubmittedScore(1U));

        const auto* pScore = harness.mockOverlayManager.GetScoreTracker(1U);
        Assert::IsNull(pScore);
    }

    TEST_METHOD(TestDoAchievementsFrameMultiple)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.MockAchievement(2U);
        harness.mockGameContext.NewLeaderboard(3U);
        harness.mockGameContext.NewLeaderboard(4U);

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 3U, 1234U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 2U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 4U, 5678U);
        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsTrue(harness.WasUnlocked(2U));

        const auto* pScore3 = harness.mockOverlayManager.GetScoreTracker(3U);
        Assert::IsNotNull(pScore3);
        Ensures(pScore3 != nullptr);
        Assert::AreEqual(std::wstring(L"1234"), pScore3->GetDisplayText());

        const auto* pScore4 = harness.mockOverlayManager.GetScoreTracker(4U);
        Assert::IsNotNull(pScore4);
        Ensures(pScore4 != nullptr);
        Assert::AreEqual(std::wstring(L"5678"), pScore4->GetDisplayText());
    }
};

} // namespace tests
} // namespace ra
