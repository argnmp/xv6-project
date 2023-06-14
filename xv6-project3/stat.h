#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
  uint seq;    // unique sequence of inode
  // for debugging
  uint ltype;
  uint target_seq;
  uint target_path_len;
  uint target_path;
};
