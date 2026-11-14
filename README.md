[![✗](https://github.com/sgiambelluca/3d-render-TLA/actions/workflows/ci.yaml/badge.svg?branch=development)](https://github.com/sgiambelluca/3d-render-TLA/actions/workflows/ci.yaml)

# Flex-Bison-Compiler

A compiler for a language that describes 3D scenes, developed with Flex and Bison. A program declares a scene with a camera, lights, materials and meshes (boxes, planes and spheres), and the compiler generates a single self-contained HTML file that draws it on a canvas, using a software rasterizer written in JavaScript. The result can be opened in any browser, without a server or additional files.

```
scene "mesa_con_jarron" {
	camera frente at (0, 3, 6) {
		look_at mesa
		fov 45deg
	}

	light sol : directional {
		direction (-1, -2, -1)
		intensity 0.9
	}

	material madera { color #8b5a2b shininess 8 }
	material vidrio { color #aee4ff80 shininess 128 }

	mesh mesa : box at (0, 0.5, 0) {
		size (2, 1, 1)
		material madera
	}

	mesh jarron : sphere at mesa.pos + (0, mesa.size.y / 2 + jarron.radius, 0) {
		radius 0.25
		material vidrio
	}

	render (1920, 1080) to "mesa.html"
}
```

* [Requirements](#requirements)
* [Configuration](#configuration)
* [Commands](#commands)
* [CI/CD](#cicd)
* [Recommended Extensions](#recommended-extensions)

## Requirements

* [Docker v29.7.2](https://www.docker.com/)

## Configuration

Set the following environment variables to control and configure the behaviour of the application:

| Name                  | Default | Description                                                                                                                                                           |
| :-------------------- | :-----: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENVIRONMENT`         | `Local` | The active environment name. The available environments are: `Local`, `Development` and `Production`.                                                                 |
| `LOG_IGNORED_LEXEMES` | `true`  | When `true`, logs all of the ignored lexemes found with Flex at `DEBUGGING` level. To remove those logs from the console output set it to `false`.                    |
| `LOGGING_LEVEL`       | `ALL`   | The minimum level to log in the console output. From lower to higher, the available levels are: `ALL`, `DEBUGGING`, `INFORMATION`, `WARNING`, `ERROR` and `CRITICAL`. |
| `OUTPUT_DIRECTORY`    | `.`     | The directory of the generated HTML file: the path in `render ... to "<file>"` is relative to it. Missing subdirectories are created.                                 |

_Docker Compose_ can read the variables from an `.env` file too (see `compose.yaml` file).

## Commands

### Start

Rises an ephemeral container, ready to start development:

```bash
docker compose run --rm compiler
```

### Build

Builds or rebuilds the entire compiler:

```bash
src/main/bash/build.sh
```

### Run

Compiles a program:

```bash
src/main/bash/run.sh <program>
```

where `<program>` is the path to the file that represents its entry-point (e.g., `programa.itba`). The compiler generates the HTML file named by the `render` entity of the scene, inside of `OUTPUT_DIRECTORY`. Open it in any browser to see the scene.

The JavaScript runtime lives in `src/main/javascript/Runtime.js`, and it's embedded in the compiler when CMake configures the project.

### Test

Executes every available unit-test under `src/test/c` folder:

```bash
src/main/bash/test.sh
```

### Stop

Logout, destroy the ephemeral containers and shutdowns the cluster:

```bash
exit
docker compose down
```

### Docker

| Command                                 | Description                                             |
| :-------------------------------------- | :------------------------------------------------------ |
| `docker builder prune --all`            | Removes all builds and complete build cache.            |
| `docker compose --progress=plain build` | Forces a build or rebuild of the images in the cluster. |
| `docker image prune`                    | Removes all of the dangling images from Docker.         |
| `docker network prune`                  | Removes unused networks from Docker.                    |
| `docker volume prune`                   | Removes unused volumes from Docker.                     |

## CI/CD

To trigger an automatic integration on every push or PR (_Pull Request_), you must activate _GitHub Actions_ in the _Settings_ tab. Use the following configuration:

| Key                                                        | Value                                               |
| :--------------------------------------------------------- | :-------------------------------------------------- |
| `Actions permissions`                                      | `Allow all actions and reusable workflows`          |
| `Allow GitHub Actions to create and approve pull requests` | `false`                                             |
| `Artifact and log retention`                               | `30 days`                                           |
| `Fork pull request workflows from outside collaborators`   | `Require approval for all outside collaborators`    |
| `Workflow permissions`                                     | `Read repository contents and packages permissions` |

## Recommended Extensions

* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
* [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* [Yash](https://marketplace.visualstudio.com/items?itemName=daohong-emilio.yash)
