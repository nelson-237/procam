from picamera import PiCamera
import sys
from argparse import Action, ArgumentParser
from os import path

# From https://pymotw.com/2/argparse/ or
# https://stackoverflow.com/questions/8632354/python-argparse-custom-actions-with-additional-arguments-passed
class AbsPathAction(Action):
    """Action to convert `argparse` arguments to absolute paths.
    """
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if nargs is not None:
            raise ValueError("`nargs` not allowed")
        super(AbsPathAction, self).__init__(option_strings, dest, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest,
                path.abspath(path.expanduser(values)))


def main():
    """Main function.
    """
    # Parse the arguments.
    # All paths are converted to *absolute* with respect to the working
    # directory from which the script is invoked.
    parser = ArgumentParser(
        description="Run Module 1 (captures visible.jpeg) from a visible light camera."
    )

    parser.add_argument('file_path',
                        help="file name of captured image from visible camera",
                        action=AbsPathAction)

    args = parser.parse_args()

    camera = PiCamera()
    camera.resolution = (1024, 768)
    camera.start_preview()
    camera.capture(args.file_path)


if __name__ == '__main__':
    sys.exit(main())

