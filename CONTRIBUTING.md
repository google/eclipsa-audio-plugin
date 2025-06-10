# Contributing to the Eclipsa Audio Plugin

Thank you for your interest in contributing to the Eclipsa Audio Plugin project! Your help is essential to making this plugin even better.

We welcome contributions from everyone. Please take a moment to review these guidelines before submitting your changes.

## About the Project

The Eclipsa Audio Plugins are a collaborative effort between [A-CX](https://www.a-cx.com/) and Google.

## How to Contribute

We’d love to accept your patches and contributions. There are just a few guidelines to follow.

### Before You Begin

#### Sign Our Contributor License Agreement (CLA)

Contributions to this project must be accompanied by a Contributor License Agreement (CLA). You (or your employer) retain the copyright to your contribution;
this simply gives us permission to use and redistribute your contributions as part of the project.

If you or your current employer have already signed the Google CLA (even if it was for a different project), you probably don't need to do it again.
Visit [https://cla.developers.google.com/](https://cla.developers.google.com/) to view your current agreements or to sign a new one.

#### Review Our Community Guidelines

This project follows [Google's Open Source Community Guidelines](https://opensource.google/conduct).

### Contribution Process

#### Code Reviews

All submissions – including those from project members – require review. We use GitHub pull requests for this purpose.
For more information on using pull requests, please refer to [GitHub Help](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests).

### Additional Guidelines for the Eclipsa Audio Plugin

#### Providing Feedback or Requesting Support

If you experience issues with the plugin, please start by consulting our documentation in the `docs/` directory.
If you believe you’ve found a bug or have suggestions for improvement, please file an issue on GitHub.
*Note: Direct email support and external resources (e.g. videos) are not used for this project.*

## Getting Started

Please see our README.md file for setting up your environment and building the plugin.

## Testing

We use CMAKE Tests to ensure that all changes are valid and all tests must pass before a change can be commited. Please ensure all changes include unit tests validating the change if possible.

## Submitting changes

Please send a Github Pull Request with a clear list of what you've done (read more about [pull requests](http://help.github.com/pull-requests/)). When you send a pull request, we appreciate if you include examples with your change, including screenshots or UI videos illustrating what has been updated. We can always use more test coverage. Please follow our coding conventions (below) and make sure all of your commits are atomic (one feature per commit).

Always write a clear log message for your commits. One-line messages are fine for small changes, but bigger changes should look like this:

    $ git commit -m "A brief summary of the commit
    > 
    > A paragraph describing what changed and its impact."


## Coding Conventions

This project uses the Google clang format for code format, as described in our .clang-format file.

In Visual Studio Code, clang formatting can automatically be applied by setting the following in the settings.json file. Note that this assumes clang-format installation using homebrew.

    "clang-format.executable": "/opt/homebrew/bin/clang-format",
    "clang-format.style": "file"

