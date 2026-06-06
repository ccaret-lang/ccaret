<p align="center">
  <a href="https://github.com/ccaret-lang/ccaret" target="_blank">
    <img src="logo.png" alt="C^ Logo" width="150"/>
  </a>
</p>

# ccaret — C^: dev-v0.1.0

[![GitHub stars][GitHubStarBadge]][GitHubUrl]
[![GitHub followers][GitHubFollowerBadge]][GitHubUrl]
[![GitHub forks][GitHubForkBadge]][GitHubUrl]
[![Discussion][DiscussionBadge]][DiscussionUrl]
<!-- [![Discord][DiscordBadge]][DiscordUrl] -->
<!-- [![Reddit][RedditBadge]][RedditUrl] -->
<!-- [![YouTube][YouTubeBadge]][YouTubeUrl] -->
<!-- [![X][XBadge]][XUrl] -->
<!-- [![Bluesky][BlueskyBadge]][BlueskyUrl] -->
<!-- [![Mastodon][MastodonBadge]][MastodonUrl] -->
<!-- [![Telegram][TelegramBadge]][TelegramUrl] -->
[![License][LicenseBadge]][LicenseUrl]

a new family member of C. C^ is neither replacement nor competitor; C^ works C, not against it.

C^ comes with value semantics, copy by default, heap is explicit. no headers, no legacy baggage. just types, pointers when you need them, and memory safety without a garbage collector.

## Philosophy

Meaningful. Accurate. Simple. Maxium Performace.

## Toolchain

`caret` is our toolchain which under the hood handles the following:
- `caretc`: this compiler
- `caretv`: version manager
- `caretp`: package manager

## Syntax

```c, cca
import std.io;

i32 main() {
  io.println("Hi, I'm C^"); // doesn't required explicity return 0; in main. still can do
}
```

## C Interop

C^ is designed to support C projects, not replace them. works directly with C. no wrappers, no bindings generators.

- `extern c {}` — declare any C function
- `import c "header.h"` — parse C headers directly
- `link "lib"` — link libraries
- `export` — make your C^ functions callable from C
- `c.` prefix always required — you always see when you're calling C

[GitHubStarBadge]: https://img.shields.io/github/stars/ccaret-lang/ccaret?style=flat&logo=github&label=Stars&logoColor=white&color=blue
[GitHubFollowerBadge]: https://img.shields.io/github/followers/ccaret-lang?style=flat&logo=github&label=Followers&logoColor=white&color=blue
[GitHubForkBadge]: https://img.shields.io/github/forks/ccaret-lang/ccaret?style=flat&logo=github&logoColor=white&label=Forks&color=blue
[DiscussionBadge]: https://img.shields.io/badge/Discussions-gray?logo=github
[DiscordBadge]: https://img.shields.io/discord/1321469752811585576?style=flat&logo=discord&logoColor=white&label=Discord&color=blue
[LicenseBadge]: https://img.shields.io/github/license/ccaret-lang/ccaret?style=flat
[GitHubUrl]: https://github.com/ccaret-lang
[DiscussionUrl]: https://github.com/ccaret-lang/ccaret/discussions
[LicenseUrl]: https://github.com/ccaret-lang/ccaret/blob/main/LICENSE
