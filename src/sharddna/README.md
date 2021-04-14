# ShardDNA
A simple C++ library for long-term, high-speed time series data storage.

*NOTE: This package is not yet complete and certainly not ready for commercial usage*

# How to say it

You can choose! 'Shard DNA', 'Shard-na' or if you're a fan of wine, you can try 'Shardonnay'.

# What is it?

ShardDNA is a library used to create a simple, efficient time-series database for long-term trending and data analysis.

When complete, the system creates database **shards** - splitting the database into discrete time series to help keep the time keys efficient. It also optimises reads when reading multiple channels simultaniously.

ShardDNA does **not** store any metadata or field information - each measurement channel has a single binary identifier (similar to a GUID) and stores information as double-precision floating point numbers. This keeps the information very simple and very clean.

The main point of difference with ShardDNA is that many other available historians simply ignore the problems with **sparse data**. Unlike many other open-source historian products (InfluxDB, Warp10, TimescaleDB etc.), ShardDNA will return the *previous and next value* around your query range. 

For example, if you have a sample recorded at 10:00am and another at 11:00am, InfluxDB will return no results if you query between 10:15 and 10:45. ShardDNA will respond with both the 10:00am and 11:00am results, allowing you to either interpolate between them or ignore the 11:00 result all-together.

# Current Functionality

Currently, ShardDNA still does not...

   * Process multi-channel queries
   * Allow channel deletion
   * Perform any aggregation or interpolation on queries
   * Create new shards when appropriate

These functions will be added in future releases.
