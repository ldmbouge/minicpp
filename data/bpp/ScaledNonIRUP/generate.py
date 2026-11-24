 
# Open the file in read mode
with open('Non_IRUP_Instances.txt', 'r') as file:
    # Read all lines from the file
    lines = file.readlines()

    # Initialize the countes
    line_number = 0
    instance_number = 0
    min_n = 1000000
    max_n = 0
    min_c = 1000000
    max_c = 0

    # Iterate over the lines
    for line in lines:
        # If the line number is divisible by 100
        if line_number % 100 == 0:            
            # Split the line into an array of integers
            numbers = list(map(int, line.strip().split()))
            # Iterate over the scales
            for s in [3,4,5]:
                # Scale the instance
                c = s * numbers[0]
                weights = sorted([s * w for w in  numbers[1:]] * s, reverse=True)
                n = len(weights)
                # Update the  counters
                instance_number += 1                
                min_c = min(min_c, c)
                min_n = min(min_n, n)
                max_c = max(max_c, c)
                max_n = max(max_n, n)
                # Write the BPP file
                file_name = "SNIRUP_N{:03}_C{:03}.BPP".format(n, c)
                with open(file_name, 'w') as file:
                    file.write("{}\n".format(n))
                    file.write("{}\n".format(c))
                    for w in weights:
                        file.write("{}\n".format(w))
        # Increment the line number counter
        line_number += 1
    
    # Print instances info
    print("Instances = {}, c = [{},{}], n = [{},{}]".format(instance_number, min_c, max_c, min_n, max_n))