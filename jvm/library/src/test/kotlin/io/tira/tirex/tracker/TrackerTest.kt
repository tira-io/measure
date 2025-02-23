package io.tira.tirex.tracker

import org.junit.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

class TrackerTest {
    @Test
    fun testProviderInfos() {
        val actual = providerInfos

        assertNotNull(actual)
        assertIs<Collection<ProviderInfo>>(actual)
        assertTrue { actual.isNotEmpty() }
    }

    @Test
    fun testMeasureInfos() {
        val actual = measureInfos

        assertNotNull(actual)
        assertIs<Map<Measure, MeasureInfo>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (measure in ALL_MEASURES) {
            assertContains(actual, measure)
        }
    }

    @Test
    fun testFetchInfo() {
        val actual = fetchInfo(setOf(Measure.OS_NAME))

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.OS_NAME)
        val resultEntry = actual[Measure.OS_NAME]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.OS_NAME)
        assertNotNull(resultEntry.type)
        assertEquals(resultEntry.type, ResultType.STRING)
        assertNotNull(resultEntry.value)
        assertTrue { resultEntry.value.isNotEmpty() }
    }

    @Test
    fun testMeasureStartAndStop() {
        val ref = startTracking(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS))
        val actual: Map<Measure, ResultEntry>
        try {
            Thread.sleep(100)
        } finally {
            actual = stopTracking(ref)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

    @Test
    fun testMeasureUsingBlock() {
        val actual = track(setOf(Measure.TIME_ELAPSED_WALL_CLOCK_MS)) {
            Thread.sleep(100)
        }

        assertNotNull(actual)
        assertIs<Map<Measure, ResultEntry>>(actual)
        assertTrue { actual.isNotEmpty() }
        for (key in actual.keys) {
            assertIs<Measure>(key)
        }
        for (value in actual.values) {
            assertIs<ResultEntry>(value)
        }
        assertContains(actual.keys, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        val resultEntry = actual[Measure.TIME_ELAPSED_WALL_CLOCK_MS]
        assertNotNull(resultEntry)
        assertNotNull(resultEntry.source)
        assertEquals(resultEntry.source, Measure.TIME_ELAPSED_WALL_CLOCK_MS)
        assertNotNull(resultEntry.type)
        //FIXME:
        //assertEquals(resultEntry.type, ResultType.FLOATING)
        assertNotNull(resultEntry.value)
        val timeElapsed = resultEntry.value.toFloat()
        assertTrue { timeElapsed > 0.0 }
    }

}
