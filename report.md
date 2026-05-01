# PES-VCS Lab Report
**Name:** Aditya Jain  
**SRN:** PES1UG24CS025  

---

## Phase 5: Branching and Checkout

### Q5.1: How would you implement `pes checkout <branch>`?

To implement `pes checkout <branch>`, the following steps are needed:

**Files that need to change in `.pes/`:**
- `.pes/HEAD` must be updated to point to the new branch: `ref: refs/heads/<branch>`
- The working directory must be updated to match the target branch's tree

**Steps:**
1. Read the target branch file at `.pes/refs/heads/<branch>` to get its commit hash
2. Read that commit object to get its tree hash
3. Read the current HEAD commit's tree hash
4. Compare the two trees to find which files differ
5. For each file that differs, overwrite the working directory file with the blob content from the target tree
6. For files that exist in the target but not the current tree, create them
7. For files that exist in the current tree but not the target, delete them
8. Update `.pes/HEAD` to `ref: refs/heads/<branch>`

**What makes this complex:**
- Nested directories require recursive tree traversal
- The operation must be atomic — if it fails halfway, the repo is in a broken state
- Uncommitted changes in the working directory may conflict with the checkout target
- Deleted files must be tracked and removed from the working directory

---

### Q5.2: How would you detect a "dirty working directory" conflict?

To detect if a file would be overwritten by checkout:

1. For each file that differs between the current branch tree and the target branch tree:
   - Look up the file in the index to get its staged blob hash and metadata (mtime, size)
   - Compare the index entry's mtime and size against the actual file on disk
   - If they differ, the file has been modified since it was last staged — this is an unstaged change
   - If the file is not in the index at all but exists on disk, it is untracked

2. If any such file would be overwritten by the checkout target's version, refuse the checkout and print an error like: `error: your local changes would be overwritten by checkout`

This works entirely using the index and object store — no need to re-hash files unless metadata suggests a change (the same optimization Git uses for speed).

---

### Q5.3: What happens if you make commits in "deta
pes branch recovery-branch
- This writes the current HEAD hash into `.pes/refs/heads/recovery-branch`
- Now those commits are reachable and safe

If you already switched away, you would need to find the commit hash from the object store (by scanning all objects) and manually create a branch pointing to it.

---

## Phase 6: Garbage Collection

### Q6.1: Algorithm to find and delete unreachable objects

**Algorithm — Mark and Sweep:**

**Mark phase (find all reachable objects):**
1. Start with all branch refs in `.pes/refs/heads/` — these are the roots
2. For each branch, read the commit hash
3. Add the commit hash to a reachable set (use a hash set for O(1) lookup)
4. Read the commit object — add its tree hash to the reachable set
5. Walk the tree recursively — add all blob and subtree hashes to the reachable set
6. Follow the parent pointer to the previous commit — repeat steps 3-6
7. Continue until reaching a commit with no parent

**Sweep phase (delete unreachable objects):**
1. Walk all files under `.pes/objects/XX/`
2. Reconstruct each object's hash from its path (first 2 chars + filename)
3. If the hash is not in the reachable set, delete the file

**Data structure:** A hash set (like a C hash table or sorted array with binary search) to store reachable hashes. Each lookup is O(1) average.

**Estimate for 100,000 commits, 50 branches:**
- Each commit has: 1 commit object + 1 root tree + ~10 blob/subtree objects on average = ~12 objects per commit
- Total reachable objects: ~1,200,000 objects to visit across all 50 branches
- Many will be shared (same files across commits), so the reachable set will be smaller
- In practice Git visits roughly 2-5x the number of commits in objects, so around 200,000-500,000 unique objects

---

### Q6.2: Race condition between GC and concurrent commit

**The race condition:**

1. A commit operation starts — it calls `tree_from_index()` and writes several blob objects to the object store. These blobs are now in `.pes/objects/` but not yet referenced by any commit or branch.

2. GC runs at this exact moment — it scans all branch refs, walks all reachable objects, and builds its reachable set. Since the new commit hasn't been written yet and `head_update()` hasn't been called, the new blobs are not reachable from any branch.

3. GC deletes those blobs as unreachable.

4. The commit operation continues — it calls `object_write()` for the commit object (which references the deleted blobs) and then calls `head_update()`. The commit is now pointed to by the branch, but the blobs it references have been deleted. The repository is now corrupt.

**How Git avoids this:**

- Git uses a "grace period" — objects newer than a certain age (default 2 weeks) are never deleted by GC, even if they appear unreachable. This gives in-progress operations time to complete.
- Git also writes a temporary ref (`.git/gc.pid` lock file) during GC so concurrent operations can detect it is running.
- The key insight is that object writes are atomic (temp+rename) and always complete before the branch ref is updated, so any object that exists on disk was either fully written or not written at all — there are no partial writes to worry about.

---

## Screenshots

*(Insert screenshots 1A, 1B, 2A, 2B, 3A, 3B, 4A, 4B, 4C, and Final here)*

