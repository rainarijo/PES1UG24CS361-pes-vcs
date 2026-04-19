# PES-VCS Lab Report

## Q5.1 — Implementing pes checkout
To implement `pes checkout <branch>`:
- Read `.pes/refs/heads/<branch>` to get the target commit hash
- Read the commit to get its tree hash
- Recursively walk the tree, writing each blob to the working directory
- Update `.pes/HEAD` to `ref: refs/heads/<branch>`
The complexity lies in handling file deletions (files in current tree but not target tree) and preserving untracked files.

## Q5.2 — Detecting Dirty Working Directory
- For each tracked file in the index, compare its current `mtime` and `size` to the stored values
- If changed, compute its SHA-256 and compare to the stored hash
- If the hash differs between the current index and the target branch's tree entry for the same file, checkout must refuse
- No extra storage needed — the index and object store have all required information

## Q5.3 — Detached HEAD
In detached HEAD state, commits are made normally but no branch reference is updated. The commits exist in the object store but become unreachable once you switch branches. Recovery: run `pes log` before switching to note the commit hash, then create a branch pointing to it: create `.pes/refs/heads/recovery` containing that hash.

## Q6.1 — Garbage Collection Algorithm
1. Start with all hashes referenced by all branch files in `.pes/refs/heads/`
2. For each commit hash: mark it, follow its parent, mark its tree, recursively mark all blobs and subtrees
3. Use a `HashSet` (hash table or sorted array) to track reachable hashes
4. List all files in `.pes/objects/`, delete any whose hash is NOT in the reachable set
For 100,000 commits × ~3 objects each (commit + tree + blobs) = roughly 300,000–500,000 objects to visit across 50 branch walks.

## Q6.2 — GC Race Condition
Race: (1) commit starts, writes a new blob but hasn't yet written the tree/commit referencing it. (2) GC runs, sees the blob is unreferenced, deletes it. (3) Commit tries to create its tree referencing the now-deleted blob — corruption.
Git avoids this by: (a) keeping "recently written" objects safe for a grace period (default 2 weeks), (b) using lock files so GC and commit cannot overlap on critical sections.
