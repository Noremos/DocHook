import numpy as np
from scipy.optimize import linear_sum_assignment
from collections import defaultdict

# Example data
expected_clusters = {
    1: 1, 2: 1, 3: 1,  # Expected cluster ID 1 for images 1, 2, 3
    4: 2, 5: 2, 6: 2   # Expected cluster ID 2 for images 4, 5, 6
}

algorithm_clusters = {
    1: 2, 2: 2, 3: 2, 4: 2,  # Algorithm outputs cluster ID 2 for images 1, 2, 3, 4
    5: 1, 6: 1               # Algorithm outputs cluster ID 1 for images 5, 6
}

# Step 1: Organize images by cluster for both expected and algorithm clusters
def organize_by_cluster(cluster_dict):
    clusters = defaultdict(list)
    for image_id, cluster_id in cluster_dict.items():
        clusters[cluster_id].append(image_id)
    return clusters

expected_cluster_groups = organize_by_cluster(expected_clusters)
algorithm_cluster_groups = organize_by_cluster(algorithm_clusters)

# Step 2: Create a cost matrix where the cost is the negative of the overlap
unique_expected_clusters = list(expected_cluster_groups.keys())
unique_algorithm_clusters = list(algorithm_cluster_groups.keys())

cost_matrix = np.zeros((len(unique_expected_clusters), len(unique_algorithm_clusters)))

for i, exp_cluster in enumerate(unique_expected_clusters):
    for j, algo_cluster in enumerate(unique_algorithm_clusters):
        overlap = len(set(expected_cluster_groups[exp_cluster]) & set(algorithm_cluster_groups[algo_cluster]))
        cost_matrix[i, j] = -overlap

# Step 3: Apply the Hungarian algorithm to find the best matching
row_ind, col_ind = linear_sum_assignment(cost_matrix)

# Create a mapping from algorithm clusters to expected clusters
mapping = {unique_algorithm_clusters[col]: unique_expected_clusters[row] for row, col in zip(row_ind, col_ind)}

# Step 4: Reassign the cluster IDs based on the mapping
corrected_algorithm_clusters = {}
for image_id, cluster_id in algorithm_clusters.items():
    corrected_algorithm_clusters[image_id] = mapping[cluster_id]

# Print the results
print("Original Algorithm Clusters:", algorithm_clusters)
print("Corrected Algorithm Clusters:", corrected_algorithm_clusters)
