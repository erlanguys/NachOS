# NachOS

[NachOS](https://en.wikipedia.org/wiki/Not_Another_Completely_Heuristic_Operating_System) is instructional software used to teach Operating Systems.

## Functionality Implemented

To be completed.

## Guides

**Create and run user programs**:

- `cd code/`
- Create the `.c` file in `userland/`
- Add the name of the program to the `PROGRAMS` variable in `userland/Makefile`
- Compile the project with `make` (in the `code` directory)
- Run it with `./os.sh up name_of_your_executable`

## Sad things you should know

1. The sole fact that NachOS is running makes it consume CPU, so don't confuse it with a userprogram actually consuming all CPU (maybe the userprogram has already finished):

![image](https://user-images.githubusercontent.com/25920622/88488581-83045580-cf64-11ea-9a57-f396a4b8dd18.png)
