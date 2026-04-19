# PES-VCS Lab Report
Screenshots

Q1
   
   a. <img width="943" height="206" alt="image" src="https://github.com/user-attachments/assets/4a1868fb-e73a-4c35-b118-844c0b90965a" />
   b. <img width="936" height="122" alt="image" src="https://github.com/user-attachments/assets/12fce07a-7c57-4a33-980c-027a6c6a162b" />
Q2. 
    
    a. <img width="936" height="282" alt="image" src="https://github.com/user-attachments/assets/3857ef8f-975a-4859-b0ce-343a5488db18" />
   b. <img width="939" height="111" alt="image" src="https://github.com/user-attachments/assets/a674cd47-ccf1-4769-bf93-f915a5e680ba" />
Q3. 
   
   a. <img width="700" height="669" alt="image" src="https://github.com/user-attachments/assets/d90fb8ce-9103-4186-a413-91d8f0b9fb6c" />
   b. <img width="695" height="62" alt="image" src="https://github.com/user-attachments/assets/1da3d062-30e9-47e5-b3cf-9c0dc9d864d8" />
Q4.
 
  a.<img width="694" height="384" alt="image" src="https://github.com/user-attachments/assets/2249f370-5be5-4d5b-9445-f406e3da73cc" />
  b. <img width="695" height="193" alt="image" src="https://github.com/user-attachments/assets/6362af80-d009-46a2-ace5-07a290128722" />
  c. <img width="699" height="92" alt="image" src="https://github.com/user-attachments/assets/ef13234e-3641-4a0e-805c-1bc05dde36a4" />

FINAL
<img width="694" height="754" alt="image" src="https://github.com/user-attachments/assets/292540b6-a97e-41e9-b076-50a6cdd9d66f" />
<img width="460" height="563" alt="image" src="https://github.com/user-attachments/assets/4ecc69a0-651e-4e6e-bc58-1e28835e06b6" />

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
