
# Table of Contents

1. [General](#general)  
		1. [Choosing the profiling task](#choosing-the-profiling-task)  
		2. [Choosing the dataset](#choosing-the-dataset)  
		3. [Task-dependent analysis](#task-dependent-analysis)   
2. [Other features](#other-features)
3. [Cite](#cite)
4. [Contacts](#contacts)

# General
Desbordante is a _data profiling_ tool. Its general usage scenario consists of five steps:
1. Choose a profiling task.
2. Choose the dataset.
3. Configure the task.
4. Run<sup>1</sup> the task.
5. Analyze the results.

<sup>1</sup>Note that tasks are computationally expensive. Due to possible heavy workload, the deployed version has memory and run time limits for all users. If your task takes up more than 512MB RAM or 30 seconds of runtime, it will be killed. Contact us if you are interested in raising the limit.

The current section covers the first two steps, while the remaining ones are specific for each profiling task.

### Choosing the profiling task

Pick one of the existing tasks.

<img style="width: 50%;" src="./images/tasks-list.png"/>


### Choosing the dataset

<div style="display: flex; column-gap: 10px; padding-bottom: 20px;">
 <div style="flex: 65%;">
  <p>
   Desbordante provides a built-in collection of datasets to test its functionality. It is not possible for unauthorized users to upload their own data, so you will need to sign up to be able to do so.
   After filling in the form you will only need to verify the provided email address.
  </p>
  <img src="./images/sign-up-button.png"/>
 </div>
 <div style="flex: 30%;"><img src="./images/sign-up-form.png"/></div>
</div>

Choose an out-of-the-box dataset or upload your own. Currently, Desbordante accepts only .csv files.

<img style="width: 30%;" src="./images/dataset-selection.png"/>

### Task-dependent analysis

Check out the corresponding profiling task guide to continue your analysis:
- [mining Functional Dependencies](guides/fd-mining.md);
- [mining Conditional Functional Dependencies](guides/cfd-mining.md);
- [mining Association Rules](guides/ar-mining.md);
- [employing the Error Detection Pipeline](guides/error-detection-pipeline.md).

# Other features
* You can return to the main screen at any time by clicking the Desbordante logo or title.
	<img style="padding-top: 15px;" src="./images/desbordante-clickable-logo.png"/>

* You can share the profiling results after the task execution finishes by copying the url with the task id.
  <img style="width: 70%; padding-top: 15px;" src="./images/share-task-results.png"/>


* We would really appreciate reviews and opinions on our tool's usability and usefulness. We would be grateful if you provided your feedback using a dedicated form which can be accessed via the button on the right.
  <img style="width: 70%; padding-top: 15px;" src="./images/feedback-form.png"/>

# Cite

If you use this software for research, please cite the paper (https://fruct.org/publications/fruct29/files/Strut.pdf, https://ieeexplore.ieee.org/document/9435469) as follows:

M. Strutovskiy, N. Bobrov, K. Smirnov and G. Chernishev, "Desbordante: a Framework for Exploring Limits of Dependency Discovery Algorithms," 2021 29th Conference of Open Innovations Association (FRUCT), 2021, pp. 344-354, doi: 10.23919/FRUCT52173.2021.9435469.

# Contacts

[Email me at strutovsky.m.a@gmail.com](mailto:strutovsky.m.a@gmail.com)
