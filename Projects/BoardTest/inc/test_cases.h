#ifndef __TEST_CASES_H__
#define __TEST_CASES_H__

typedef int( TestCaseHandler )( int argc, char *argv[] );

typedef struct TestCaseSt_ {
    char name[32];
    TestCaseHandler* fn;
}TestCaseSt;


int test_case_ctxcw(int argc, char *argv[]);
int test_case_crx(int argc, char *argv[]);
int test_case_crxs(int argc, char *argv[]);
int test_case_ctx(int argc, char *argv[]);
int test_case_csleep(int argc, char *argv[]);
int test_case_cstdby(int argc, char *argv[]);
int test_case_cmculpm(int argc, char *argv[]);

#endif //__TEST_CASES_H__