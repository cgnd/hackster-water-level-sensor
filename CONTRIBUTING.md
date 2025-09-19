# Contributing

Contributions are welcome!

## Developer Tools

### uv

Some of the developer tools described below are provided as python packages. Commands are provided for installing these tools using [uv](https://docs.astral.sh/uv/), an extremely fast Python package and project manager.

Follow the [installation guide](https://docs.astral.sh/uv/getting-started/installation/) to install the `uv` command.

> [!NOTE]
>
> While `uv` is recommended, it's not required. The tools described below can also be installed via other python package managers such as [pipx](https://pipx.pypa.io/stable/) or [pip](https://pip.pypa.io/en/stable/).

### pre-commit

This project uses [pre-commit](https://pre-commit.com/) to catch simple issues before a commit is created in the repository.

Run the following commands to install the `pre-commit` tool and the git pre-commit hooks defined by this project's [`.pre-commit-config.yaml`](.pre-commit-config.yaml) file:

```sh
uv tool install pre-commit
pre-commit install
```

You can configure `git` to automatically install `pre-commit` hooks when a new repository is cloned (rather than having to run `pre-commit install` manually). Follow the recommended setup instructions:

https://pre-commit.com/#pre-commit-init-templatedir

### REUSE

This project uses the [`reuse`](https://reuse.software/) tool to ensure that all files have copyright and license information. REUSE compliance is checked automatically via a pre-commit hook, but it's also possible to install the `reuse` tool and run the linter manually on all files in the repository:

```
uv tool install reuse
reuse lint
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

Place both lines at the very top of the file using the file’s native comment syntax. If you authored substantial, original content, you *may* add an additional copyright line for yourself or your organization.

### Developer Certification of Origin (DCO) Sign-Off

When you contribute to this project, you certify that your contribution is made under the terms of the [Developer Certificate of Origin (DCO)](https://developercertificate.org/), without any additional terms or conditions.

This project requires a `Signed-off-by:` trailer line added to each git commit log message certifying that your contribution complies with the DCO.

```plaintext
Signed-off-by: Your Name <your.email@example.com>
```

For your commits, replace:

- `Your Name` with your legal name (pseudonyms, hacker handles, and the names of groups are not allowed)
- `your.email@example.com` with the real email address you are using to author the commit. Pseudo or anonymized emails such as `you-id+your-username@users.noreply.github.com` are not allowed. The email must match the one you use to author the commit (CI will fail if there is no match).

You can automatically add the `Signed-off-by` trailer to your commit body using `git commit -s` (see the git documentation for [`--signoff`](https://git-scm.com/docs/git-commit#Documentation/git-commit.txt--s)). If you are altering an existing commit created by someone else, you must add your `Signed-off-by:` line without removing the existing one.
