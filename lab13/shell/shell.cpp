#include "shell.h"
#include "../cstdio.h"
#include "../cstdlib.h"

Shell::Shell()
{
    initialize();
}
void Shell::initialize()
{
    head = end = 0;
}

void Shell::run()
{
    const char *user = "root@nelson-cheung.cn # ";

    clear();
    printGraphSymbol();
    while (true)
    {

        printf("%s", user);
        inputAndShow();   // 等待用户输入
        extractCommand(); // 提取用户命令

        // 处理内部命令
        if (strlib::strcmp((char *)cmd, SHELL_LS) == 0)
        {
            printCurrentDirectory();
        }
        else if (strlib::strcmp((char *)cmd, SHELL_TREE) == 0)
        {
            PCB *process = sysProgramManager.running();
            printCurrentDirectoryTree(0, process->currentDirectory);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_CD) == 0)
        {
            cd((char *)parameter);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_MKDIR) == 0)
        {
            createFile((char *)parameter, DIRECTORY_FILE);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_NELSON) == 0)
        {
            printGraphSymbol();
        }
        else if (strlib::strcmp((char *)cmd, SHELL_EXEC) == 0)
        {
            printf("you enter command \"%s\"\n", cmd);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_RM) == 0)
        {
            rm((char *)parameter);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_CAT) == 0)
        {
            printf("you enter command \"%s\"\n", cmd);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_ECHO) == 0)
        {
            printf("you enter command \"%s\"\n", cmd);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_TOUCH) == 0)
        {
            createFile((char *)parameter, REGULAR_FILE);
        }
        else if (strlib::strcmp((char *)cmd, SHELL_PWD) == 0)
        {
            PCB *process = sysProgramManager.running();
            printf("%s\n", process->currentDirectory.getName());
        }
        else
        {
            printf("command \"%s\" is not supported\n", cmd);
        }

        head = end = 0;
    }
}

void Shell::clear()
{
    dword length = 80 * 25;
    moveCursor(0);
    for (dword i = 0; i < length; ++i)
    {
        putc(0);
    }
    moveCursor(0);
}

void Shell::printGraphSymbol()
{
    printf("\n");
    printf("          ________   _______      ___    ___ ________  ________\n");
    printf("         |\\   ___  \\|\\  ___ \\    |\\  \\  /  /|\\   __  \\|\\   ___  \n");
    printf("         \\ \\  \\\\ \\  \\ \\   __/|   \\ \\  \\/  / | \\  \\|\\  \\ \\  \\\\ \\  \\\n");
    printf("          \\ \\  \\\\ \\  \\ \\  \\_|/__  \\ \\    / / \\ \\  \\\\\\  \\ \\  \\\\ \\  \\\n");
    printf("           \\ \\  \\\\ \\  \\ \\  \\_|\\ \\  /     \\/   \\ \\  \\\\\\  \\ \\  \\\\ \\  \\ \n");
    printf("            \\ \\__\\\\ \\__\\ \\_______\\/  /\\   \\    \\ \\_______\\ \\__\\\\ \\__\\\n");
    printf("             \\|__| \\|__|\\|_______/__/ /\\ __\\    \\|_______|\\|__| \\|__|\n");
    printf("                                 |__|/ \\|__|                         \n\n");
}

void Shell::inputAndShow()
{
    byte code;
    dword counter = 0;
    byte c;

    while (1)
    {
        while (!sysKeyboard.pop(&code))
        {
        }

        if (code & 0x80)
        {
            continue;
        }

        c = sysKeyboard.scanCode2Char(code);
        switch (c)
        {
        // 特殊按键
        case 0:
        {
            switch (code)
            {
            case KEY_SPACES:
            {
                putc(' ');
                ++counter;
                buffer[end] = ' ';
                end = (end + 1) % SHELL_BUFFER_SIZE;
            }
            break;

            default:
                break;
            }
        }
        break;

        case '\b':
        {
            dword pos = getCursor();
            if (pos && counter)
            {
                --counter;
                --end;
                moveCursor(pos - 1);
                putc(0);
                moveCursor(pos - 1);
            }
        }
        break;

        case '\t':

            break;

        case '\r':
        {
            do
            {
                putc(0);
            } while (getCursor() % 80);
            return;
        }
        break;

        default:
        {
            putc(c);
            ++counter;
            buffer[end] = c;
            end = (end + 1) % SHELL_BUFFER_SIZE;
        }
        break;
        }
    }
}

void Shell::extractCommand()
{
    memset(cmd, '\0', SHELL_COMMAND_SIZE + 1);

    dword i = 0;
    while ((head != end) &&
           (i < SHELL_COMMAND_SIZE) &&
           (buffer[head] != ' '))
    {
        cmd[i] = buffer[head];
        head = (head + 1) % SHELL_BUFFER_SIZE;
        ++i;
    }

    // 命令太长，截断处理
    while ((head != end) && (buffer[head] != ' '))
    {
        head = (head + 1) % SHELL_BUFFER_SIZE;
    }

    if ((head != end))
    {
        head = (head + 1) % SHELL_BUFFER_SIZE;
    }
}

void Shell::printCurrentDirectory()
{
    PCB *process = sysProgramManager.running();
    if (process->pageDir)
    {
        Inode inode = sysFileSystem.getInode(process->currentDirectory.inode);
        DirectoryEntry entry;
        dword size = 0;
        while (size < inode.size)
        {
            inode.read(size, &entry, sizeof(DirectoryEntry));
            printf("%s\n", entry.getName());
            size += sizeof(DirectoryEntry);
        }
    }
    printf("\n");
}

void Shell::printCurrentDirectoryTree(dword level, const DirectoryEntry &dir)
{
    for (int i = 0; i < level; ++i)
    {
        printf("  ");
    }

    printf("|-%s\n", dir.name);

    if (dir.type != DIRECTORY_FILE ||
        strlib::strcmp(dir.name, ".") == 0 ||
        strlib::strcmp(dir.name, "..") == 0)
        return;

    Inode inode = sysFileSystem.getInode(dir.inode);

    DirectoryEntry entry;

    dword size = 0;
    while (size < inode.size)
    {
        inode.read(size, &entry, sizeof(DirectoryEntry));
        printCurrentDirectoryTree(level + 1, entry);
        size += sizeof(DirectoryEntry);
    }
}

void Shell::extractNextParameter()
{
    dword i = 0;
    while ((head != end) &&
           (buffer[head] != ' '))
    {
        parameter[i] = buffer[head];
        head = (head + 1) % SHELL_BUFFER_SIZE;
        ++i;
    }

    parameter[i] = 0;

    if (head != end)
    {
        head = (head + 1) % SHELL_BUFFER_SIZE;
    }
}

DirectoryEntry Shell::getDirectoryOfFile(const char *path)
{
    dword inodeNumber = 0;
    DirectoryEntry entry;
    char filename[MAX_FILE_NAME + 1];

    dword first, last, next, len;
    first = strlib::firstIn(path, '/');
    last = strlib::lastIn(path, '/');

    DirectoryEntry current;

    if (first == 0)
    {
        current.inode = 0;
        current.type = DIRECTORY_FILE;
    } else if(last == -1) {
        return sysProgramManager.running()->currentDirectory;
    }
    else
    {
        current = sysProgramManager.running()->currentDirectory;
    }

    // 允许多个'/'整合成为一个'/'
    while (first < last && path[first] == '/')
        ++first;

    while (first < last)
    {
        next = strlib::firstIn(path + first, '/') + first;

        // 非法文件名
        if (next - first > MAX_FILE_NAME)
        {
            printf("invalid file name\n");
            return DirectoryEntry();
        }
        strlib::strcpy(path, filename, first, next - first);
        //printf("file name: %s\n", filename);

        current = sysFileSystem.getEntryInDirectory(current, filename, DIRECTORY_FILE);

        // 不存在此目录
        if (current.inode == -1)
            return current;

        first = next + 1;
        while (first < last && path[first] == '/')
            ++first;
    }

    return current;
}

void Shell::getFileNameInPath(const char *path, char *filename)
{
    int start = strlib::lastIn(path, '/');
    if (start < 0)
        start = 0;
    else
        start += 1;

    strlib::strcpy(path, filename, start, strlib::len(path) - start);
}

void Shell::createFile(const char *path, dword type)
{
    extractNextParameter();
    DirectoryEntry entry = getDirectoryOfFile((char *)parameter);
    printf("%d %s\n", entry.inode, entry.getName());
    if (entry.inode != -1)
    {
        char filename[MAX_FILE_NAME + 1];
        getFileNameInPath((char *)parameter, filename);
        dword ans = sysFileSystem.createEntryInDirectory(entry, filename, type);
        if (ans == -1)
        {
            printf("\"%s\" can not be created\n", parameter);
        }
    }
    else
    {
        printf("\"%s\" can not be created\n", parameter);
    }
}

void Shell::rm(const char *path)
{
    extractNextParameter();
    DirectoryEntry entry = getDirectoryOfFile((char *)parameter);
    if (entry.inode != -1)
    {
        char filename[MAX_FILE_NAME + 1];
        getFileNameInPath((char *)parameter, filename);
        dword ans = sysFileSystem.deleteEntryInDirectory(entry, filename, DIRECTORY_FILE);
        if (ans == -1)
        {
            printf("\"%s\" can not be deleted\n", parameter);
        }
    }
    else
    {
        printf("\"%s\" can not be deleted\n", parameter);
    }
}

void Shell::cd(const char *path)
{
    extractNextParameter();
    PCB *process = sysProgramManager.running();
    if (strlib::strcmp(path, ".") == 0)
    {
    }
    else if (strlib::strcmp(path, "..") == 0)
    {
    }
    else
    {
        DirectoryEntry entry = getDirectoryOfFile(path);
        if (entry.inode != -1)
        {
            char filename[MAX_FILE_NAME + 1];
            getFileNameInPath(path, filename);
            entry = sysFileSystem.getEntryInDirectory(entry, filename, DIRECTORY_FILE);
            if (entry.inode == -1)
            {
                printf("\"%s\" does not exist\n", parameter);
            }
            else
            {
                sysProgramManager.running()->currentDirectory = entry;
            }
        }
        else
        {
            printf("\"%s\" does not exist\n", parameter);
        }
    }
}