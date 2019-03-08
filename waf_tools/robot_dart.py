"""
Quick n dirty robot_dart detection
"""

import os
from waflib import Utils, Logs
from waflib.Configure import conf


def options(opt):
  opt.add_option('--robot_dart', type='string', help='path to robot_dart', dest='robot_dart')


@conf
def check_robot_dart(conf, *k, **kw):
    def get_directory(filename, dirs):
        res = conf.find_file(filename, dirs)
        return res[:-len(filename)-1]

    required = kw.get('required', False)

    includes_check = ['/usr/local/include', '/usr/include']
    libs_check = ['/usr/local/lib', '/usr/lib']

    # OSX/Mac uses .dylib and GNU/Linux .so
    lib_suffix = 'dylib' if conf.env['DEST_OS'] == 'darwin' else 'so'

    # # You can customize where you want to check
    # # e.g. here we search also in a folder defined by an environmental variable
    # if 'RESIBOTS_DIR' in os.environ:
    # 	includes_check = [os.environ['RESIBOTS_DIR'] + '/include'] + includes_check
    # 	libs_check = [os.environ['RESIBOTS_DIR'] + '/lib'] + libs_check

    if conf.options.robot_dart:
        includes_check = [conf.options.robot_dart + '/include']
        libs_check = [conf.options.robot_dart + '/lib']

    try:
        conf.start_msg('Checking for robot_dart includes')
        dirs = []
        dirs.append(get_directory('robot_dart/robot.hpp', includes_check))
        dirs.append(get_directory('robot_dart/control/robot_control.hpp', includes_check))
        dirs.append(get_directory('robot_dart/robot_dart_simu.hpp', includes_check))
        dirs.append(get_directory('robot_dart/descriptor/base_descriptor.hpp', includes_check))

        # remove duplicates
        dirs = list(set(dirs))

        conf.end_msg(dirs)
        conf.env.INCLUDES_ROBOT_DART = dirs

        conf.start_msg('Checking for robot_dart library')
        libs_ext = ['.a', lib_suffix]
        lib_found = False
        type_lib = '.a'
        for lib in libs_ext:
            try:
                lib_dir = get_directory('libRobotDARTSimu' + lib, libs_check)
                lib_found = True
                type_lib = lib
                break
            except:
                lib_found = False
        conf.end_msg('libRobotDARTSimu' + type_lib)

        conf.env.LIBPATH_ROBOT_DART = lib_dir
        if type_lib == '.a':
            conf.env.STLIB_ROBOT_DART = 'RobotDARTSimu'
        else:
            conf.env.LIB_ROBOT_DART = 'RobotDARTSimu'
    except:
        if required:
            conf.fatal('Not found')
        conf.end_msg('Not found', 'RED')
        return
    return 1