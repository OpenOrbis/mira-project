Shamelessly Stolen and Modified from RPCS3's Repo: https://github.com/RPCS3/rpcs3/wiki/coding-style

We recommend to follow these guidelines when writing code for Mira-Project. They aren't very strict rules since we want to be flexible and we understand that under certain circumstances some of them can be counterproductive. Just try to follow as many of them as possible:

### General coding style
* Variable naming: *Prefix + CamelCase*
    * Globals: _g__*
    * Class members: _m__*
    * Scoped: _s__*
    * Looped: _l__*
    * Constants: _c__*
* Template parameter names: *CamelCase*, or just T, U, V...
* Avoid `#defines`, use constant variables instead, preferrably in an enum class.
* Put curly-brackets (`{` and `}`) on the next line.
* Eliminate all compiler warnings from your code.
* Comment *every* hack you do, *every* snippet you comment out and *every* improvable code.
* If you have to comment or place a commented code snippet, include the reasons to do that in the comment.
* Ensure that every source file you modify has the newline at the end of file. Every line ends with "newline" and the end of file must have "newline" too, GitHub usually warns about it.