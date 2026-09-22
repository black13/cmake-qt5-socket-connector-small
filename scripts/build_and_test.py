"""Configure, build, and run the headless regression tests."""
import argparse
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', default='build_tests')
    parser.add_argument('--config', choices=('Debug', 'Release'), default='Debug')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--libxml2-source', type=Path,
                        help='Reuse an existing libxml2 checkout for an offline build')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    build = (root / args.build_dir).resolve()

    # An explicit mapping removes duplicate case variants (PATH/Path) from
    # inherited Windows environment blocks, which otherwise break MSBuild.
    environment = dict(os.environ)
    if os.name == 'nt':
        environment['MSBUILDDISABLENODEREUSE'] = '1'

    def run(command):
        subprocess.run(command, cwd=root, env=environment, check=True)

    configure = ['cmake', '-S', str(root), '-B', str(build),
                 '-DBUILD_TESTING=ON', f'-DCMAKE_BUILD_TYPE={args.config}',
                 f'-DFETCHCONTENT_BASE_DIR={build / "_deps"}']
    if args.libxml2_source:
        source = args.libxml2_source.resolve()
        if not (source / 'CMakeLists.txt').is_file():
            parser.error('--libxml2-source must contain a libxml2 checkout')
        configure.append(f'-DFETCHCONTENT_SOURCE_DIR_LIBXML2={source}')
    run(configure)
    run(['cmake', '--build', str(build), '--config', args.config,
         '--parallel', str(args.jobs)])
    run(['ctest', '--test-dir', str(build), '-C', args.config,
         '--output-on-failure'])


if __name__ == '__main__':
    main()
