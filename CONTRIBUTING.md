# Contributing

Contributions are welcome!

<!-- mdformat-toc start --slug=github --no-anchors --maxlevel=6 --minlevel=1 -->

- [Contributing](#contributing)
  - [Developer Tools](#developer-tools)
    - [uv](#uv)
    - [pre-commit](#pre-commit)
    - [pre-commit-hooks](#pre-commit-hooks)
    - [REUSE](#reuse)
    - [yamllint](#yamllint)
    - [markdownlint-cli2](#markdownlint-cli2)
    - [mdformat](#mdformat)
    - [taplo](#taplo)
  - [Licensing](#licensing)
    - [Copyright and License Notices](#copyright-and-license-notices)
    - [Developer Certification of Origin (DCO) Sign-Off](#developer-certification-of-origin-dco-sign-off)

<!-- mdformat-toc end -->

## Developer Tools

This project uses [pre-commit](https://pre-commit.com/) to identify simple issues before submission to code review. Although `pre-commit` hooks are run automatically in CI, it is strongly recommended that you install `pre-commit` locally to catch these issues *before* a commit is created in the repository.

### uv

Some of the developer tools described below (e.g. `pre-commit`) are installed via python packages. Commands are provided below for installing these tools using [uv](https://docs.astral.sh/uv/), an extremely fast Python package and project manager.

Follow the [uv installation guide](https://docs.astral.sh/uv/getting-started/installation/) to install the `uv` command.

> [!NOTE]
> While `uv` is recommended, it's not required. The python-based tools described below can also be installed globally via other python package managers such as [pipx](https://pipx.pypa.io/stable/), or installed directly into your python environment via [pip](https://pip.pypa.io/en/stable/).

### pre-commit

Run the following commands to install the `pre-commit` tool and the git pre-commit hooks defined by this project's [`.pre-commit-config.yaml`](.pre-commit-config.yaml) file:

```sh
uv tool install pre-commit
pre-commit install

# Run all pre-commit hooks on all files in the repository
pre-commit run --all
```

You can configure `git` to automatically install `pre-commit` hooks when a new repository is cloned (rather than having to run `pre-commit install` manually). Follow the recommended setup instructions:

<https://pre-commit.com/#pre-commit-init-templatedir>

### pre-commit-hooks

The pre-commit developers provide an [official set of pre-commit hooks](https://github.com/pre-commit/pre-commit-hooks). These hooks are installed and run automatically via `pre-commit`, but it's also possible to install them so each hook can be run as a command outside of the pre-commit framework:

```sh
uv tool install pre-commit-hooks

# Check a json file
check-json .vscode/extensions.json
```

### REUSE

This project uses the [`reuse`](https://reuse.software/) tool to ensure that all files have copyright and license information. REUSE compliance is checked automatically via a pre-commit hook, but it's also possible to install the `reuse` CLI tool and run the linter manually on all files in the repository:

```sh
uv tool install reuse

# Lint all files in the project
reuse lint
```

### yamllint

[yamllint](https://yamllint.readthedocs.io/en/stable/index.html) is a linter for YAML files. It is run automatically as a pre-commit hook, but it's also possible to install the `yamllint` CLI tool and run the linter manually.

```sh
uv tool install yamllint

# Lint YAML files in the current directory tree
yamllint .
```

### markdownlint-cli2

[markdownlint-cli2](https://github.com/DavidAnson/markdownlint-cli2) is a command-line interface for linting Markdown files. It is run automatically as a pre-commit hook, but it's also possible to install the `markdownlint-cli2` CLI tool and run the linter manually.

Follow the [Install](https://github.com/DavidAnson/markdownlint-cli2?tab=readme-ov-file#install) guide to install the `markdownlint-cli2` tool.

```sh
# Lint markdown files in the current directory tree
markdownlint-cli2 .
```

### mdformat

[mdformat](https://mdformat.readthedocs.io/en/stable/index.html) is an opinionated Markdown formatter that can be used to enforce a consistent style in Markdown files. It is run automatically as a pre-commit hook, but it's also possible to install the `mdformat` CLI tool and run the formatter manually.

```sh
uv tool install \
--with mdformat-toc \
--with mdformat-gfm \
--with mdformat-frontmatter \
--with mdformat-footnote \
--with mdformat-gfm-alerts \
--with mdformat-tables \
mdformat

# Format markdown files in the current directory tree
mdformat .
```

### taplo

taplo is a toolkit for TOML files that supports linting and formatting. It is run automatically as a pre-commit hook, but it's also possible to install the `taplo` CLI tool and run the linter/formatter manually.

Follow the [Installation](https://taplo.tamasfe.dev/cli/installation/binary.html) guides to install the `taplo` tool.

```sh
# Lint TOML files in the current directory tree
taplo lint

# Format TOML files in the current directory tree
taplo format
```

## Licensing

See [LICENSE.md](LICENSE.md) for details on this project's licensing information.

### Copyright and License Notices

This project uses [SPDX](https://spdx.dev/)/[REUSE](https://reuse.software/)-style file headers:

<!-- REUSE-IgnoreStart -->

```plaintext
SPDX-FileCopyrightText: Common Ground Electronics <https://cgnd.dev>
SPDX-License-Identifier: <SPDX License ID>
```

<!-- REUSE-IgnoreEnd -->

Place both lines at the very top of the file using the file’s native comment syntax (make sure to replace `<SPDX License ID>` with the actual [SPDX License ID](https://spdx.org/licenses/)). If you authored substantial, original content, you *may* add an additional copyright line for yourself or your organization.

### Developer Certification of Origin (DCO) Sign-Off

When you contribute to this project, you certify that your contribution is made under the terms of the [Developer Certificate of Origin (DCO)](https://developercertificate.org/), without any additional terms or conditions.

This project requires a `Signed-off-by:` trailer line added to each git commit log message certifying that your contribution complies with the DCO.

```plaintext
Signed-off-by: Your Name <your.email@example.com>
```

For your commits, replace:

- `Your Name` with your legal name (pseudonyms, hacker handles, and the names of groups are not allowed)
- `your.email@example.com` with the real email address you are using to author the commit.

You can automatically add the `Signed-off-by` trailer to your commit body using `git commit -s` (see the git documentation for [`--signoff`](https://git-scm.com/docs/git-commit#Documentation/git-commit.txt--s)). If you are altering an existing commit created by someone else, you must add your `Signed-off-by:` line without removing the existing one.
