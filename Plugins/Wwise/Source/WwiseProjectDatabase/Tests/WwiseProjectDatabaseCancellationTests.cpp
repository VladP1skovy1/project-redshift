/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal

License Usage

Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2026 Audiokinetic Inc.
*******************************************************************************/

#include "Wwise/WwiseUnitTests.h"

#if WWISE_UNIT_TESTS
#include "Wwise/AdapterTypes/IWwiseDirectoryVisitor.h"
#include "Wwise/WwiseProjectDatabaseCancellationToken.h"
#include "Wwise/WwiseProjectDatabaseImpl.h"
#include "Wwise/WwiseStringConverter.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <atomic>

WWISE_TEST_CASE(WwiseProjectDatabase_CancellationTokenNullToken, "Wwise::WwiseProjectDatabase::CancellationToken::NullToken", "[ApplicationContextMask][SmokeFilter]")
{
    SECTION("Default-constructed (null) token never cancels")
    {
        FWwiseProjectDatabaseCancellationToken Token;
        WWISE_CHECK_FALSE(Token.IsCancelled());
    }
}

WWISE_TEST_CASE(WwiseProjectDatabase_CancellationTokenMatchingGeneration, "Wwise::WwiseProjectDatabase::CancellationToken::MatchingGeneration", "[ApplicationContextMask][SmokeFilter]")
{
    SECTION("Token whose expected generation matches the live value is not cancelled")
    {
        std::atomic<uint32> Generation{ 3 };
        FWwiseProjectDatabaseCancellationToken Token{ &Generation, 3 };
        WWISE_CHECK_FALSE(Token.IsCancelled());
    }
}

WWISE_TEST_CASE(WwiseProjectDatabase_CancellationTokenAdvancingGeneration, "Wwise::WwiseProjectDatabase::CancellationToken::AdvancingGeneration", "[ApplicationContextMask][SmokeFilter]")
{
    SECTION("Advancing the live generation cancels an older token")
    {
        std::atomic<uint32> Generation{ 3 };
        FWwiseProjectDatabaseCancellationToken Token{ &Generation, 3 };
        Generation.fetch_add(1, std::memory_order_relaxed);
        WWISE_CHECK(Token.IsCancelled());
    }
}

WWISE_TEST_CASE(WwiseProjectDatabase_ImplGenerationTokenMonotonic, "Wwise::WwiseProjectDatabase::GenerationToken::MonotonicToken", "[ApplicationContextMask][SmokeFilter]")
{
    SECTION("BumpUpdateGeneration always increases and the exposed token stays in sync")
    {
        FWwiseProjectDatabaseImpl Impl;
        const std::atomic<uint32>* Token = Impl.GetUpdateGenerationToken();
        WWISE_CHECK(Token != nullptr);
        WWISE_CHECK(Token->load(std::memory_order_relaxed) == 0);

        const uint32 Gen1 = Impl.BumpUpdateGeneration();
        WWISE_CHECK(Gen1 == 1);
        WWISE_CHECK(Token->load(std::memory_order_relaxed) == 1);

        const uint32 Gen2 = Impl.BumpUpdateGeneration();
        WWISE_CHECK(Gen2 == 2);
        WWISE_CHECK(Token->load(std::memory_order_relaxed) == 2);
    }
}

namespace WwiseProjectDatabaseCancellationTests
{
    class FCancelAfterNVisitor
    {
    public:
        FCancelAfterNVisitor(std::atomic<uint32>* InGeneration, int32 InCancelAfter)
            : Generation(InGeneration), CancelAfter(InCancelAfter)
        {}

        bool Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory)
        {
            ++VisitCount;
            if (VisitCount >= CancelAfter)
            {
                Generation->fetch_add(1, std::memory_order_relaxed);
            }
            return true;
        }

        int32 VisitCount = 0;

    private:
        std::atomic<uint32>* Generation;
        int32 CancelAfter;
    };
}

WWISE_TEST_CASE(WwiseProjectDatabase_DirectoryVisitorAbortsMidWalk, "Wwise::WwiseProjectDatabase::DirectoryVisitor::AbortsMidWalk", "[ApplicationContextMask][SmokeFilter]")
{
    using namespace WwiseProjectDatabaseCancellationTests;

    SECTION("IterateDirectory stops visiting entries as soon as the token is superseded")
    {
        const FString TempDir = FPaths::Combine(FPlatformProcess::UserTempDir(), TEXT("WwiseProjectDatabaseCancelTest"));
        IFileManager::Get().MakeDirectory(*TempDir, true);
        for (int32 i = 0; i < 6; ++i)
        {
            FFileHelper::SaveStringToFile(TEXT("x"), *FPaths::Combine(TempDir, FString::Printf(TEXT("File%d.txt"), i)));
        }

        std::atomic<uint32> Generation{ 0 };
        FWwiseProjectDatabaseCancellationToken Token{ &Generation, 0 };

        FCancelAfterNVisitor* RawVisitor = new FCancelAfterNVisitor(&Generation, 2);
        IWwiseDirectoryVisitor<FCancelAfterNVisitor> Visitor(RawVisitor, Token);
        Visitor.IterateDirectory(FWwiseStringConverter::ToWwiseDBString(TempDir));

        WWISE_CHECK(Token.IsCancelled());
        WWISE_CHECK(RawVisitor->VisitCount == 2);

        delete RawVisitor;
        IFileManager::Get().DeleteDirectory(*TempDir, false, true);
    }
}
    
WWISE_TEST_CASE(WwiseProjectDatabase_DirectoryVisitorContinueIfSameGeneration, "Wwise::WwiseProjectDatabase::DirectoryVisitor::ContinueIfSameGeneration", "[ApplicationContextMask][SmokeFilter]")
{
    using namespace WwiseProjectDatabaseCancellationTests;

    SECTION("IterateDirectory visits every entry when never cancelled")
    {
        const FString TempDir = FPaths::Combine(FPlatformProcess::UserTempDir(), TEXT("WwiseProjectDatabaseCancelTest2"));
        IFileManager::Get().MakeDirectory(*TempDir, true);
        for (int32 i = 0; i < 4; ++i)
        {
            FFileHelper::SaveStringToFile(TEXT("x"), *FPaths::Combine(TempDir, FString::Printf(TEXT("File%d.txt"), i)));
        }

        std::atomic<uint32> Generation{ 0 };
        FWwiseProjectDatabaseCancellationToken Token{ &Generation, 0 };

        // CancelAfter beyond the entry count: never trips.
        FCancelAfterNVisitor* RawVisitor = new FCancelAfterNVisitor(&Generation, /*CancelAfter=*/100);
        IWwiseDirectoryVisitor<FCancelAfterNVisitor> Visitor(RawVisitor, Token);
        Visitor.IterateDirectory(FWwiseStringConverter::ToWwiseDBString(TempDir));

        WWISE_CHECK_FALSE(Token.IsCancelled());
        WWISE_CHECK(RawVisitor->VisitCount == 4);

        delete RawVisitor;
        IFileManager::Get().DeleteDirectory(*TempDir, false, true);
    }
}
#endif // WWISE_UNIT_TESTS
