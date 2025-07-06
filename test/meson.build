gtest = dependency('gtest', main : true, disabler : true, required : false)

concepts_test = executable(
    'concepts_test',
    'concepts_test.cpp',
    cpp_args : '-fconcepts-diagnostics-depth=2',
    include_directories : '..',
    link_with : [sdbus],
    dependencies : [
        gtest,
    ],
)

sig_test = executable(
    'sig_test',
    'sig_test.cpp',
    cpp_args : '-fconcepts-diagnostics-depth=2',
    include_directories : '..',
    link_with : [sdbus],
    dependencies : [
        gtest,
    ],
)

rw_test = executable(
    'read_write_test',
    'read_write_test.cpp',
    cpp_args : '-fconcepts-diagnostics-depth=2',
    include_directories : '..',
    link_with : [sdbus],
    dependencies : [
        gtest,
        systemd_dep,
    ],
)

test('concepts', concepts_test)
test('signature', sig_test)
test('read_write', rw_test)