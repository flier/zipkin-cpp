#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef GFLAGS_NAMESPACE
using namespace GFLAGS_NAMESPACE;
#else
using namespace gflags;
#endif

int main(int argc, char **argv)
{
    ParseCommandLineFlags(&argc, &argv, false);
    ::testing::GTEST_FLAG(throw_on_failure) = true;
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
