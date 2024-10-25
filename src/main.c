#include "system.h"
#include "mapper.h"
#include "emulator.h"

const char* doc_str =
	"nes-tools is an NES emulator.\n\n"
	"Usage:\n\n"
	"\tnes-tools <command> [arguments]\n\n"
	"The commands are:\n\n"
	"\trun\tRun the emulator on a given ROM\n"
	"\tversion\tOutput the nes-tools version\n\n"
	"Use \"nes-tools help <command>\" for more information about a command.\n";

int run(int argc, char** argv)
{
	if (argc < 2) {
		LOG(ERROR, "\"run\" command expected ROM path as argument");
		printf("Run '%s help run' for usage.\n", PACKAGE_NAME);
		exit(EXIT_FAILURE);
	}

	mapper_t* mapper;
	if (!(mapper = mapper_from_file(argv[1])))
		exit(EXIT_FAILURE);

	emulator_t* emu;
	if (!(emu = emulator_create(mapper))) {
		mapper_destroy(mapper);
		exit(EXIT_FAILURE);
	}

	emulator_exec(emu);

	LOG(INFO, "Play time %d min", (uint64_t)emu->time_diff / 60000);
	LOG(INFO, "Frame rate: %.4f fps", (double)(emu->ppu->frames * 1000) / emu->time_diff);
	LOG(INFO, "Audio sample rate: %.4f Hz", (double)(emu->apu->sampler.samples * 1000) / emu->time_diff);
	LOG(INFO, "CPU clock speed: %.4f MHz", ((double)emu->cpu->t_cycles / (1000 * emu->time_diff)));

	emulator_destroy(emu);
	mapper_destroy(mapper);

	return 0;
}

int version()
{
	printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	return 0;
}

int help(int argc, char** argv)
{
	if (argc < 2) {
		printf("%s", doc_str);
		exit(EXIT_SUCCESS);
	}

	if (!strcmp(argv[1], "run")) {
		printf("usage: %s run [NES ROM File]\n\n", PACKAGE_NAME);
		printf("Runs the specified NES ROM file. Only iNES file format is currently accepted.\n");
		exit(EXIT_SUCCESS);
	}

	if (!strcmp(argv[1], "version")) {
		printf("usage: %s version\n\n", PACKAGE_NAME);
		printf("Outputs the current %s package version\n", PACKAGE_NAME);
		exit(EXIT_SUCCESS);
	}

	if (!strcmp(argv[1], "help")) {
		printf("usage: %s help\n\n", PACKAGE_NAME);
		printf("Outputs the %s help string\n", PACKAGE_NAME);
		exit(EXIT_SUCCESS);
	}

	LOG(ERROR, "unrecognized help topic: %s", argv[1]);
	printf("Run '%s help' for usage.\n", PACKAGE_NAME);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		LOG(ERROR, "need at least (1) argument");
		printf("Run '%s help' for usage.\n", PACKAGE_NAME);
		exit(EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "run"))
		return run(argc - 1, &argv[1]);

	if (!strcmp(argv[1], "version"))
		return version();

	if (!strcmp(argv[1], "help"))
		return help(argc - 1, &argv[1]);

	LOG(ERROR, "Unrecognized command");
	printf("Run '%s help' for usage.\n", PACKAGE_NAME);
	exit(EXIT_FAILURE);
}
