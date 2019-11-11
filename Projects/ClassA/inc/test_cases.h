#ifndef __TEST_CASES_H__
#define __TEST_CASES_H__

typedef int( TestCaseHandler )( char* str );

typedef struct TestCaseSt_ {
    char name[32];
    TestCaseHandler* fn;
}TestCaseSt;


int test_case_ctxcw(char* str);

#endif //__TEST_CASES_H__