/* test bound checking code without -run */

int arr[10];

int
main(int argc, char **argv)
{
  int i;

  for (i = 0; i <= sizeof(arr)/sizeof(arr[0]); i++)
        arr[i] = 0;
  return 0;
}
