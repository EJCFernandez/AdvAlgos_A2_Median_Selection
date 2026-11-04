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
|3/11|Claude|Write the benchmark harness (bench.cpp)|Batched identical copies of one input, so the branch predictor learned median-of-medians' and nth_element's branch patterns and made them look about 3–5x faster below n = 10^4. Quickselect was unaffected because of its random pivots. The flawed results looked convincing|Claude noticed a jump at the batching boundary in fig1 and confirmed it with a micro-benchmark (identical copies about 3.5× faster than distinct inputs)|Switched to batches of distinct inputs and reran everything. The flawed run is kept in results_v1_batching_artifact/ I then manually reviewed the fix|
|3/11|Claude|Counted-build settings in the harness|Used only 3 repetitions for the counted build, too few for quickselect's widely varying comparison counts, so fig2 and fig6 were noisy|Pure intuition and got Claude to double check as the results didn't seem natural to me|Raised to 50 repetitions, plotted as means with standard-error bars|
|4/11|Claude|Draft Section 4 from my results|Several numbers came from its own notes rather than the data: it called comparison-count slopes "time" slopes, said the Lomuto median-of-medians cap was n = 2,000–4,000 (it is 2,000), gave p99 as 1.67× (the figure shows 1.66×) and said median-of-medians costs "exactly" 3.5n on all-equal input (only at large n)|I looked at summary.csv and compared them to the numbers on the draft and found things weren't matching up so then got Claude to confirm|Corrected all four before merging Section 4|

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
| 3 Nov | Claude | Drafted Section 2.1–2.3 (comparison tables, constant factors, input sensitivity) | Read and edited the whole draft myself.|
| 3 Nov | Claude | Wrote eight prediction questions and rewrote my answers in report style | All predictions are my own answers; I checked the rewording kept my meaning and edited it where needed |
| 3 Nov | Claude | Verified APA 7 in-text citations and a reference list | Claude checked each one by web search (titles, pages, DOIs). |
| 3 Nov | Claude | Wrote the benchmark harness (bench.cpp, five modes, seeding, batching, time cap) and scripts/reproduce.sh | reviewed the code, ran the smoke test and full run, every result checked against nth_element |
| 3 Nov | Claude | Wrote scripts/plot.py (seven figures, summary.csv, tail_summary.txt, dups_slopes.csv)| Reviewed results and worked with my intuition of good results and the theory and with Claude to confirm |
| 3 Nov | Claude | Drafted Section 1 from my notes and Section 2, then fixed four sentences where my grammar edits had changed the meaning | Proof read and edited it and edited the grammar myself and checked every cited source |
| 4 Nov | Claude | Combined the report sections into one Word document and matched the formatting | I got Claude to double check word by word that no text changed and confirmed myself by using word count and a quick pass |
| 4 Nov | Claude | Drafted Section 3 (methodology) from my project notes | Proof read it in its entirety and edited anything I came across |
| 4 Nov | Claude | Drafted Section 4 (results and analysis) and checked its numbers against summary.csv and the figures | I inserted the figures and proof read it in its entirety, checked the numbers and edited anything I came across |
| 4 Nov | Claude | Explained branch prediction and caching when I could not explain my two surprises myself, then drafted Section 5 from my answers | My expectations and reactions in Section 5 are my own, and I made sure I understood the explanation before accepting it and then I proof read section 5 in its entirety and edited anything I came across|
| 4 Nov | Claude | Drafted Section 6 | Proof read it in its entirety and edited and added anything I saw fit |
| 4 Nov | Claude | Drafted the Video script by working in collaboration with me and going back and forth on versions | Proof read each one in its entirety and edited and added anything I saw fit and then went back and forth with Claude until I was happy |
| 4 Nov | Claude | Drafted the Slides in Powerpoint based on the script | Looked through each slide and made sure they looked nice and made sense before doing any recording |
| 4 Nov | Claude | Create an updated ReadMe | Proof read and added links where necessary |
| 4 Nov | Claude | Help me write Section 8 by using the ai-log and by going back and forth on questions regarding my own personal experiences | Proof read and edited the resulting text |