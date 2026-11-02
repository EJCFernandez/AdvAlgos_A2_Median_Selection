# AI use log

Log as you go. The report needs a personal paragraph on how you used AI,
**three or more specific cases where it was wrong, unhelpful or misleading**,
and a reflection (the spec's "AI in group work" question; adapt per the
coordinator's answer).

## Problems (wrong, unhelpful, misleading)

| Date | Tool | What I asked | What it got wrong | How I found out | What I did |
|---|---|---|---|---|---|
|27/10|Claude|Check tone, spelling and grammar for a text extract|Made corrections to the text that were completly off the appropriate tone and somewhat garbled the flow in the process|Once asked I read through the resultant text that Claude replied with and noticed the mistakes|Ignored Claude's advice and decided to rely on my own intuition and writing ability|
|28/10|Claude|How adaptations of a given algorithm can be implemented into Claude's given skeleton|Gave me median-of-medians adaptation advice using Lomuto partitioning and reported that it passed all tests and that it was correct when in reality while maybe technically correct the presentation of correctness was misleading and in reality the test takes way to long due to key duplicates (15+ minutes and it still didn't finish instead ran ctrl c)|My own test run was taking far to long and my intuition told me something was obviously wrong. So I confronted Claude about it and it eventually found the issue.|Switched both algorithms to three-way partitioning (Lomuto kept for comparison) and all tests passed in seconds.|

## General use

| Date | Tool | Used for | Did I verify? How? |
|---|---|---|---|
| 27 Oct | Claude | Topic choice help, plan, repo skeleton (CMake, test harness, stubs) | Treated these as drafts which require human validation to confirm correctness. Part by Part I'll be confirming correctness whenever I use something from these areas. |
| 28 Oct | Claude | Quickselect, partitioning and Lomuto explanation | Cross referencing with self research from different sites such as https://www.geeksforgeeks.org/dsa/quickselect-algorithm/ and https://stackoverflow.com/questions/10861388/quickselect-algorithm-simplified-explanation |
| 28 Oct | Claude | Wrote the n − 1 comparison-count test (Skeleton area) | Temporarily reverting to a[i] <= pivot and confirming it fails basically just confirms that the test is valid by confirming it fails when given something technically incorrect |
| 28 Oct | Claude | Median-of-medians general explanation and help adapting algorithms to specifics in the skeleton | Cross referencing with self research https://www.geeksforgeeks.org/dsa/kth-smallest-largest-element-in-unsorted-array-worst-case-linear-time/ and https://stackoverflow.com/questions/9489061/understanding-median-of-medians-algorithm also full suite testing|
|28 Oct | Claude | Investigated slow test runs from median-of-medians and explained why it was happening| Matched my own observation of debug with info being presented and implementing the three-way fix made it work|
|28 Oct | Claude | Helped implement the three-way partitioning and updated the tests. I typed the algorithm changes myself| Test results all checked out with the new code 114,400 checks and 0 failures |
|28 Oct | Claude | Further project notes and planning| Read through and verified with my own knowledge of the project|